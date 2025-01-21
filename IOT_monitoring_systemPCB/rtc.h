#ifndef IOT_MONITORING_SYSTEM_RTC_H_
#define IOT_MONITORING_SYSTEM_RTC_H_

#include "ESP32Time.h"

typedef struct
{
  int hours;
  int minutes;
  int seconds;
  int days;
  int months;
  int years;

} DATE_t;

class IOT_rtc
{
  private:
    ESP32Time *_rtc;  // offset in seconds GMT+1
    DATE_t _date_time;
  public:
    IOT_rtc();
    IOT_rtc(DATE_t date_time);
    IOT_rtc(int hours, int minutes, int seconds, int days, int months, int years);
    void begin();
    void begin(int hours, int minutes, int seconds, int days, int months, int years);
    void set_time(int hours, int minutes, int seconds, int days, int months, int years);
    int get_seconds();
    int get_minutes();
    int get_hours();
    int get_day();
    int get_month();
    int get_year();
    void print_get_data_time(String format = "%H:%M:%S-%d/%b/%Y");
    String get_data_time(String format = "%H:%M:%S--%d/%b/%Y");
};

#endif