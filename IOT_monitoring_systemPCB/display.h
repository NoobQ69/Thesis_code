#ifndef IOT_MONITORING_SYSTEM_DISPLAY_H_
#define IOT_MONITORING_SYSTEM_DISPLAY_H_

#define DSP_RX_PIN 33
#define DSP_TX_PIN 32

// #define DSP_RX_PIN 25
// #define DSP_TX_PIN 33

class IOT_display
{
  private:

  public:
    IOT_display();
    int begin();
    void send_data_to_display(const char *data);
    void print_page_on_display(int page_num);
    int get_data_from_display(char *buffer);
};

#endif