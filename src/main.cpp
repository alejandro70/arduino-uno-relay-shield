#include <Arduino.h>
#include "SimpleTimer.h"

#define SPEED_MODES 4

int relayPin = 3;
int ledPin = 4;
int buttonPin = PIN_A0;
int relayState = LOW;
int mode = 0;
long periodTime = 10 * 1000;                     // 3 * 60 * 1000;                // periodo 3 minutos
long dutyCycle[SPEED_MODES] = {0, 100, 66, 33}; // % de tiempo señal activa en un periodo
int buttonState;
int lastButtonState = HIGH;         // the previous reading from the input pin
unsigned long lastDebounceTime = 0; // the last time the output pin was toggled
unsigned long debounceDelay = 50;   // the debounce time; increase if the output flickers
SimpleTimer timer;
int timerPeriod;
int timerOff[SPEED_MODES];
 
void toggleRelay()
{
  relayState = relayState == LOW ? HIGH : LOW;
  digitalWrite(relayPin, relayState);
  digitalWrite(ledPin, relayState);
}

void setRelay(int state)
{
  relayState = state;
  digitalWrite(relayPin, state);
  digitalWrite(ledPin, state);
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

void disableAll()
{
  timer.disable(timerPeriod);
  for (size_t i = 0; i < SPEED_MODES; i++)
  {
    timer.disable(timerOff[i]);
  }
}

void startCycle()
{
  setRelayOn();
  timer.restartTimer(timerPeriod);  
  timer.restartTimer(timerOff[mode]);
}

void nextMode()
{
  disableAll();
  mode = mode < SPEED_MODES - 1 ? mode + 1 : 0;

  if (mode == 0)
  {
    setRelay(LOW);
    return;
  }

  if (mode == 1)
  {
    setRelayOn();
    return;
  }

  timer.enable(timerPeriod);
  timer.enable(timerOff[mode]);
  startCycle();
}

void setup()
{
  Serial.begin(9600);

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(relayPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  digitalWrite(ledPin, LOW);
  digitalWrite(relayPin, LOW);

  timerPeriod = timer.setInterval(periodTime, startCycle);
  for (size_t i = 0; i < SPEED_MODES; i++)
  {
    long activeTime = dutyCycle[i] * (periodTime / 100L);
    Serial.println(activeTime);
    timerOff[i] = timer.setInterval(activeTime, setRelayOff);
  }

  mode = 0;
  disableAll();
}

void loop()
{
  if (buttonPressed())
    nextMode();

  timer.run();
}