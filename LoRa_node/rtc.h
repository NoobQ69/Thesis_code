#ifndef IOT_MONITORING_SYSTEM_RTC_H_
#define IOT_MONITORING_SYSTEM_RTC_H_

#include "uRTCLib.h"

#define RTC_DS3202

typedef struct
{
  int hours;
  int minutes;
  int seconds;
  int days;
  int months;
  int years;

} DATE_t;

class Rtc
{
  private:
    uRTCLib *_rtc;
    DATE_t _date_time;
  public:
    Rtc();
    Rtc(DATE_t date_time);
    Rtc(int hours, int minutes, int seconds, int days, int months, int years);
    void begin();
    void begin(int hours, int minutes, int seconds, int days, int months, int years);
    void set_time(int hours, int minutes, int seconds, int days, int months, int years);
    int get_seconds();
    int get_minutes();
    int get_hours();
    void print_get_data_time();
    String get_data_time(String format = "%A, %B %d %Y %H:%M:%S");
};

#endif