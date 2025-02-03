#ifndef IOT_MONITORING_SYSTEM_RTC_H_
#define IOT_MONITORING_SYSTEM_RTC_H_

#include <Wire.h> // must be included here so that Arduino library object file references work
#include <RtcDS3231.h>
/* for normal hardware wire use above */

// #define RTC_DS3202

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
    // uRTCLib *_rtc;
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
    int get_day();
    int get_month();
    int get_year();
    void get_date_time_string(char *string_to_store);
    void print_date_time();
    void print_date_time(const RtcDateTime& dt);
    String get_data_time(String format = "%A, %B %d %Y %H:%M:%S");
};

#endif