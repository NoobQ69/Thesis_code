#include <Arduino.h>
#include "actuators.h"

IOT_actuators::IOT_actuators()
{
  this->_fan_1_2 = FAN_1_PIN;
  this->_cylinder_1 = CYLINDER_1_PIN;
  this->_cylinder_2 = CYLINDER_2_PIN;
  // this->_fan_2 = FAN_2_PIN;
  this->_pump_1 = PUMP_1_PIN;
}

// IOT_actuators::IOT_actuators(int fan_1_pin, int fan_2_pin, int pump_1_pin)
// {
//   this->_fan_1 = fan_1_pin;
//   this->_fan_2 = fan_2_pin;
//   this->_pump_1 = pump_1_pin;
// }

int  IOT_actuators::get_cylinder_1_pins()
{
  return this->_cylinder_1;
}

int  IOT_actuators::get_cylinder_2_pins()
{
  return this->_cylinder_2;
}

void IOT_actuators::begin()
{
  pinMode(this->_fan_1_2, OUTPUT);
  pinMode(this->_cylinder_1, OUTPUT);
  pinMode(this->_cylinder_2, OUTPUT);
  // pinMode(this->_fan_2, OUTPUT);
  // pinMode(this->_pump_1, OUTPUT);
  // pinMode(this->_motor_1, OUTPUT);
  // pinMode(this->_switch_1, INPUT_PULLUP);
  Serial.println("Actuators begin");
}

void IOT_actuators::set_actuators_pin(int pin, int mode)
{
  if (this->_pump_1 == pin)
  {
    digitalWrite(this->_pump_1, mode);
  }
  else if (this->_fan_1_2 == pin)
  {
    digitalWrite(this->_fan_1_2, mode);
  }
  else if (this->_cylinder_1 == pin)
  {
    digitalWrite(this->_cylinder_1, mode);
  }
  else if (this->_cylinder_2 == pin)
  {
    digitalWrite(this->_cylinder_2, mode);
  }
}

int IOT_actuators::read_actuator_pins(int pin)
{
  if (this->_switch_1 == pin)
  {
    return digitalRead(this->_switch_1);
  }
  return 0;
}
