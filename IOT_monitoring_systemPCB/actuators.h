#ifndef IOT_MONITORING_SYSTEM_ACTUATORS_H_
#define IOT_MONITORING_SYSTEM_ACTUATORS_H_

#define FAN_1_PIN 14
#define FAN_A_PIN 26
#define PUMP_1_PIN 14
#define CYLINDER_1_PIN 12
#define CYLINDER_2_PIN 13

// #define FAN_1_PIN 13
// #define FAN_A_PIN 12
// #define PUMP_1_PIN 14
// #define CYLINDER_1_PIN 32
// #define CYLINDER_2_PIN 27

// #define SWITCH_1_PIN 14

class IOT_actuators
{
  private:
    // int _fan_1;
    int _fan_1_2;
    int _pump_1;
    int _switch_1;
    int _cylinder_1;
    int _cylinder_2;
  public:
    IOT_actuators();
    // IOT_actuators(int fan_1_pin, int fan_2_pin, int pump_1_pin);
    void begin();
    void set_actuators_pin(int pin, int mode);
    int  read_actuator_pins(int pin);
    int  get_cylinder_1_pins();
    int  get_cylinder_2_pins();
};

#endif