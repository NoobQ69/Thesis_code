#include <Arduino.h>
#include "FunctionalInterrupt.h"
#include "buttons.h"

IOT_buttons::IOT_buttons()
{
  
}

void IOT_buttons::begin()
{
  pinMode(BUTTON_1, INPUT);
  pinMode(BUTTON_2, INPUT);
  pinMode(BUTTON_3, INPUT);
  pinMode(BUTTON_4, INPUT);
  Serial.println("Buttons begins");
}

void IOT_buttons::begin(int pin, int mode = INPUT_PULLUP, void (*isr_btn)() = NULL, int mode_interrupt = RISING)
{
  pinMode(pin, mode);

  if (isr_btn != NULL)
  {
    attachInterrupt(pin, isr_btn, mode_interrupt);
  }

  Serial.println("Buttons begins");
}

void IOT_buttons::begin(int arr[], int size, int mode = INPUT_PULLUP)
{
  for (int i = 0; i < size; i++)
  {
    pinMode(arr[i], mode);
    digitalWrite(arr[i], HIGH);
  }
  
  Serial.println("Buttons begins");
}

int IOT_buttons::read(int button, void (*callback_func)() = NULL)
{
  if (digitalRead(button) == LOW)
  {
    while (digitalRead(button) == LOW)
    {
      vTaskDelay(50);
    }

    if (callback_func != NULL)
    {
      callback_func();  // don't put delay to callback function!
    }
    return ON;
  }
  return OFF;
}

