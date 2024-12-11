#ifndef IOT_MONITORING_SYSTEM_DISPLAY_H_
#define IOT_MONITORING_SYSTEM_DISPLAY_H_

#define DPS_RX_PIN 27
#define DPS_TX_PIN 14

class Display
{
  private:

  public:
    Display();
    int begin();
    void send_data_to_display(const char *data);
    void print_page_on_display(int page_num);
    int get_data_from_display(char *buffer);
};

#endif