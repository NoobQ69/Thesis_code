#include "rtc.h"
#include "ESP32Time.h"

IOT_rtc::IOT_rtc()
{
  this->_date_time.hours    = 9;
  this->_date_time.minutes  = 0;
  this->_date_time.seconds  = 0;
  this->_date_time.days     = 1;
  this->_date_time.months   = 8;
  this->_date_time.years    = 2024;
}

IOT_rtc::IOT_rtc(DATE_t date_time)
{
  this->_date_time = date_time;
}

IOT_rtc::IOT_rtc(int hours, int minutes, int seconds, int days, int months, int years)
{
  this->_date_time.hours     = hours;
  this->_date_time.minutes   = minutes;
  this->_date_time.seconds   = seconds;
  this->_date_time.days      = days;
  this->_date_time.months    = months;
  this->_date_time.years     = years;
}

void IOT_rtc::begin()
{
  this->_rtc = new ESP32Time(0);  // offset in seconds GMT+7
  this->_rtc->setTime(this->_date_time.seconds, 
              this->_date_time.minutes,
              this->_date_time.hours,
              this->_date_time.days,
              this->_date_time.months,
              this->_date_time.years);  // 17th Jan 2021 15:24:30

  Serial.println("RTC begins");
}

void IOT_rtc::begin(int hours, int minutes, int seconds, int days, int months, int years)
{
  this->_rtc = new ESP32Time(0);  // offset in seconds GMT+1
  this->_rtc->setTime(seconds,
              minutes,
              hours,
              days,
              months,
              years);  // 17th Jan 2021 15:24:30

  Serial.println("RTC begins");
}

void IOT_rtc::set_time(int hours, int minutes, int seconds, int days, int months, int years)
{
  this->_rtc->setTime(seconds,
              minutes, 
              hours, 
              days, 
              months, 
              years);
}

int IOT_rtc::get_seconds()
{
  return this->_rtc->getSecond();
}

int IOT_rtc::get_minutes()
{
  return this->_rtc->getMinute();
}

int IOT_rtc::get_hours()
{
  return this->_rtc->getHour(true);
}

int IOT_rtc::get_day()
{
  return this->_rtc->getDay();
}

int IOT_rtc::get_month()
{
  return this->_rtc->getMonth();
}

int IOT_rtc::get_year()
{
  return this->_rtc->getYear();
}

void IOT_rtc::print_get_data_time(String format)
{
  Serial.println(this->_rtc->getTime(format));
}

String IOT_rtc::get_data_time(String format)
{
  return this->_rtc->getTime(format);
}
