#include <Arduino.h>
#include "SimpleTimer.h"
#include <stdarg.h>

#define SERIAL_PRINTF_MAX_BUFF 256

int relayPin = 3;
int redPin = 4;
int greenPin = PIN_A1;
int buttonPin = PIN_A0;
int relayState = LOW;
int mode = 0;
long periodTime = 1L * 60L * 1000L;     // periodo del ciclo de trabajo
long dutyCycle[] = {0, 100, 66}; // % de tiempo señal activa en un periodo
size_t numModes;
int buttonState;
int lastButtonState = HIGH;         // the previous reading from the input pin
unsigned long lastDebounceTime = 0; // the last time the output pin was toggled
unsigned long debounceDelay = 50;   // the debounce time; increase if the output flickers
SimpleTimer timer;
int counterShowMode;
int msShowMode = 50;
int timerPeriod;
int timerNextMode;
int timerBlinkGreen;
int timerShowMode;
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
void showMode();
void startCycle();
void timerStart(int numTimer);
void toggleGreen();
void log();

void setup()
{
  Serial.begin(9600);

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(relayPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  digitalWrite(redPin, LOW);
  digitalWrite(greenPin, LOW);
  relayState = LOW;
  digitalWrite(relayPin, relayState);

  numModes = sizeof(dutyCycle) / sizeof(long);
  timerOff = new int[numModes];

  //timerLog = timer.setInterval(1000, log);
  timerNextMode = timer.setInterval(1000, nextMode);
  timerBlinkGreen = timer.setInterval(100, toggleGreen);
  timerShowMode = timer.setInterval(msShowMode, showMode);
  timerPeriod = timer.setInterval(periodTime, startCycle);
  for (size_t i = 0; i < numModes; i++)
  {
    long activeTime = (long)dutyCycle[i] * (long)(periodTime / 100L);
    timerOff[i] = timer.setInterval(activeTime, setRelayOff);
  }

  setMode(0);
}

void loop()
{
  if (buttonPressed())
  {
    nextMode();
    //timerStart(timerNextMode);
    //timerStart(timerBlinkGreen);
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
  timer.disable(timerBlinkGreen);
  timer.disable(timerPeriod);
  for (size_t i = 0; i < numModes; i++)
  {
    timer.disable(timerOff[i]);
  }
}

void showMode()
{
  bool on = (mode == 0 && counterShowMode == 0);
  on = on || mode == 1;
  on = on || (mode > 1 && counterShowMode * msShowMode < dutyCycle[mode] * 10);
  digitalWrite(greenPin, on ? HIGH : LOW);
  counterShowMode = counterShowMode * msShowMode < 1000 ? counterShowMode + 1 : 0;
}

void toggleGreen()
{
  if (digitalRead(greenPin) == LOW)
    digitalWrite(greenPin, HIGH);
  else
    digitalWrite(greenPin, LOW);
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
