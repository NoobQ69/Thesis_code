#ifndef IOT_MONITORING_SYSTEM_BUTTONS_H_
#define IOT_MONITORING_SYSTEM_BUTTONS_H_

#define BUTTON_1    39
#define BUTTON_2    36
#define BUTTON_3    35
#define BUTTON_4    34

class IOT_buttons
{
  private:

  public:
    IOT_buttons();
    void begin();
    void begin(int pin, int mode, void (*isr_btn)(), int mode_interrupt);
    void begin(int arr[], int size, int mode);
    int  read(int button, void (*callback_func)());

};

#endif
