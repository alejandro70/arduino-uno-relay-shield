#include <Arduino.h>
#include "SimpleTimer.h"
#include <stdarg.h>

#define SERIAL_PRINTF_MAX_BUFF 256

int relayPin = 3;
int redPin = 4;
int buttonPin = PIN_A0;
int relayState = LOW;
int mode = 0;
long periodTime = 10 * 1000;         // periodo 3 minutos
long dutyCycle[] = {0, 100, 66, 33}; // % de tiempo señal activa en un periodo
size_t numModes;
int buttonState;
int lastButtonState = HIGH;         // the previous reading from the input pin
unsigned long lastDebounceTime = 0; // the last time the output pin was toggled
unsigned long debounceDelay = 50;   // the debounce time; increase if the output flickers
SimpleTimer timer;
int timerPeriod;
int timerNextMode;
int timerBlinkRed;
int timerLog;
int *timerOff;

bool buttonPressed();
void disableTimers();
void nextMode();
void setMode(int newMode);
void setRelay(int state);
void setRelayOff();
void setRelayOn();
void serialPrintf(const char *fmt, ...);
void startCycle();
void timerStart(int numTimer);
void toggleRed();
void log();

void setup()
{
  Serial.begin(9600);

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(redPin, OUTPUT);
  pinMode(relayPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  digitalWrite(redPin, LOW);
  relayState = LOW;
  digitalWrite(relayPin, relayState);

  numModes = sizeof(dutyCycle) / sizeof(long);
  timerOff = new int[numModes];
  
  //timerLog = timer.setInterval(1000, log);
  timerNextMode = timer.setInterval(1000, nextMode);
  timerBlinkRed = timer.setInterval(100, toggleRed);
  timerPeriod = timer.setInterval(periodTime, startCycle);
  for (size_t i = 0; i < numModes; i++)
  {
    long activeTime = dutyCycle[i] * (periodTime / 100L);
    timerOff[i] = timer.setInterval(activeTime, setRelayOff);
  }

  setMode(0);
}

void loop()
{
  if (buttonPressed())
  {
    timerStart(timerNextMode);
    timerStart(timerBlinkRed);
  }

  timer.run();
}

void log()
{
  serialPrintf("relayState=%d,mode=%d,maxModes=%d,buttonState=%d,lastButtonState=%d\n", relayState, mode, numModes, buttonState, lastButtonState);
}

void nextMode()
{
  int newMode = mode < numModes - 1 ? mode + 1 : 0;
  setMode(newMode);
}

void setMode(int newMode)
{
  mode = newMode;
  disableTimers();
  if (mode == 0)
  {
    setRelayOff();
  }
  else if (mode == 1)
  {
    setRelayOn();
  }
  else
  {
    startCycle();
  }
}

void startCycle()
{
  setRelayOn();
  timerStart(timerPeriod);
  timerStart(timerOff[mode]);
}

void setRelay(int state)
{
  relayState = state;
  digitalWrite(relayPin, state);
  digitalWrite(redPin, state);
}

void setRelayOn()
{
  setRelay(HIGH);
}

void setRelayOff()
{
  setRelay(LOW);
}

bool buttonPressed()
{
  bool pressed = false;
  int reading = digitalRead(buttonPin);
  if (reading != lastButtonState)
  {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay)
  {
    if (reading != buttonState)
    {
      buttonState = reading;
      if (buttonState == LOW)
      {
        pressed = true;
      }
    }
  }

  lastButtonState = reading;

  return pressed;
}

void disableTimers()
{
  timer.disable(timerNextMode);
  timer.disable(timerBlinkRed);
  timer.disable(timerPeriod);
  for (size_t i = 0; i < numModes; i++)
  {
    timer.disable(timerOff[i]);
  }
}

void toggleRed()
{
  if (digitalRead(redPin) == LOW)
    digitalWrite(redPin, HIGH);
  else
    digitalWrite(redPin, LOW);
}

void timerStart(int numTimer)
{
  timer.enable(numTimer);
  timer.restartTimer(numTimer);
}

void serialPrintf(const char *fmt, ...)
{
  /* Buffer for storing the formatted data */
  char buff[SERIAL_PRINTF_MAX_BUFF];
  /* pointer to the variable arguments list */
  va_list pargs;
  /* Initialise pargs to point to the first optional argument */
  va_start(pargs, fmt);
  /* create the formatted data and store in buff */
  vsnprintf(buff, SERIAL_PRINTF_MAX_BUFF, fmt, pargs);
  va_end(pargs);
  Serial.print(buff);
}
