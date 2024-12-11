#include "rtc.h"


// uRTCLib rtc(0x68);

Rtc::Rtc()
{
  this->_rtc = new uRTCLib(0x68);
  this->_date_time.hours    = 9;
  this->_date_time.minutes  = 0;
  this->_date_time.seconds  = 0;
  this->_date_time.days     = 1;
  this->_date_time.months   = 8;
  this->_date_time.years    = 2024;
}

Rtc::Rtc(DATE_t date_time)
{
  this->_rtc = new uRTCLib(0x68);
  this->_date_time = date_time;
}

Rtc::Rtc(int hours, int minutes, int seconds, int days, int months, int years)
{
  this->_rtc = new uRTCLib(0x68);
  this->_date_time.hours     = hours;
  this->_date_time.minutes   = minutes;
  this->_date_time.seconds   = seconds;
  this->_date_time.days      = days;
  this->_date_time.months    = months;
  this->_date_time.years     = years;
}

void Rtc::begin()
{
  this->_rtc->set_model(URTCLIB_MODEL_DS3231);
  // this->_rtc = new ESP32Time(3600);  // offset in seconds GMT+1
  // this->_rtc->setTime(this->_date_time.seconds, 
  //             this->_date_time.minutes, 
  //             this->_date_time.hours, 
  //             this->_date_time.days, 
  //             this->_date_time.months, 
  //             this->_date_time.years);  // 17th Jan 2021 15:24:30
  // rtc.set(0, 42, 16, 6, 2, 5, 15);
	//  RTCLib::set(byte second, byte minute, byte hour, byte dayOfWeek, byte dayOfMonth, byte month, byte year)
  URTCLIB_WIRE.begin();
  this->_rtc->set(this->_date_time.seconds, 
              this->_date_time.minutes, 
              this->_date_time.hours, 
              0,
              this->_date_time.days, 
              this->_date_time.months, 
              this->_date_time.years);  // 17th Jan 2021 15:24:30

  Serial.println("RTC begins");
}

void Rtc::begin(int hours, int minutes, int seconds, int days, int months, int years)
{
  // this->_rtc = new ESP32Time(3600);  // offset in seconds GMT+1
  URTCLIB_WIRE.begin();
  this->_rtc->set(seconds, 
              minutes, 
              hours,
              0,
              days, 
              months, 
              years);  // 17th Jan 2021 15:24:30

  Serial.println("RTC begins");
}

void Rtc::set_time(int hours, int minutes, int seconds, int days, int months, int years)
{
  this->_rtc->set(seconds, 
              minutes, 
              hours,
              0,
              days, 
              months, 
              years);
}

int Rtc::get_seconds()
{
  return this->_rtc->second();
}

int Rtc::get_minutes()
{
  return this->_rtc->minute();
}

int Rtc::get_hours()
{
  return this->_rtc->hour();
}

void Rtc::print_get_data_time()
{
  int time_arr[6];
  time_arr[0] = this->_rtc->hour();
  time_arr[1] = this->_rtc->minute();
  time_arr[2] = this->_rtc->second();
  time_arr[3] = this->_rtc->day();
  time_arr[4] = this->_rtc->month();
  time_arr[5] = this->_rtc->year();
  Serial.printf("%d:%d:%d-%d/%d/%d");
}

// String Rtc::get_data_time(String format)
// {
//   return this->_rtc->getTime(format);
// }
