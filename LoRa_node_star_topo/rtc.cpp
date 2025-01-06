#include "rtc.h"
#include "utilities.h"
#define countof(a) (sizeof(a) / sizeof(a[0]))
RtcDS3231<TwoWire> Rtc1(Wire);
String MonthsOfTheYear[] = {"","Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
/* for normal hardware wire use above */

// uRTCLib rtc(0x68);

Rtc::Rtc()
{
  // this->_rtc = new uRTCLib(0x68);
  this->_date_time.hours    = 9;
  this->_date_time.minutes  = 0;
  this->_date_time.seconds  = 0;
  this->_date_time.days     = 1;
  this->_date_time.months   = 8;
  this->_date_time.years    = 2024;
}

Rtc::Rtc(DATE_t date_time)
{
  // this->_rtc = new uRTCLib(0x68);
  this->_date_time = date_time;
}

Rtc::Rtc(int hours, int minutes, int seconds, int days, int months, int years)
{
  // this->_rtc = new uRTCLib(0x68);
  this->_date_time.hours     = hours;
  this->_date_time.minutes   = minutes;
  this->_date_time.seconds   = seconds;
  this->_date_time.days      = days;
  this->_date_time.months    = months;
  this->_date_time.years     = years;
}

// handy routine to return true if there was an error
// but it will also print out an error message with the given topic
static bool wasError(const char* errorTopic = "")
{
    uint8_t error = Rtc1.LastError();
    if (error != 0)
    {
        // we have a communications error
        // see https://www.arduino.cc/reference/en/language/functions/communication/wire/endtransmission/
        // for what the number means
        Serial.print("[");
        Serial.print(errorTopic);
        Serial.print("] WIRE communications error (");
        Serial.print(error);
        Serial.print(") : ");

        switch (error)
        {
        case Rtc_Wire_Error_None:
            Serial.println("(none?!)");
            break;
        case Rtc_Wire_Error_TxBufferOverflow:
            Serial.println("transmit buffer overflow");
            break;
        case Rtc_Wire_Error_NoAddressableDevice:
            Serial.println("no device responded");
            break;
        case Rtc_Wire_Error_UnsupportedRequest:
            Serial.println("device doesn't support request");
            break;
        case Rtc_Wire_Error_Unspecific:
            Serial.println("unspecified error");
            break;
        case Rtc_Wire_Error_CommunicationTimeout:
            Serial.println("communications timed out");
            break;
        }
        return true;
    }
    return false;
}

static String convert_to_rtc_date_init_format(int month, int day, int year)
{
  String date_str = "";
  date_str += MonthsOfTheYear[month];
  date_str += " ";
  date_str += String(day);
  date_str += " ";
  date_str += String(year);

  return date_str;
}

static String get_current_time_String_format(int hour, int minute, int second) 
{
  String time_str = "";
  time_str = String(hour);
  time_str += ":";
  time_str += String(minute);
  time_str += ":";
  time_str += String(second);

  return time_str;
}

static int rtc_setup(RtcDateTime &compiled)
{
  if (!Rtc1.IsDateTimeValid()) 
  {
    if (!wasError("setup IsDateTimeValid"))
    {
      // Common Causes:
      //    1) first time you ran and the device wasn't running yet
      //    2) the battery on the device is low or even missing

      Serial.println("RTC lost confidence in the DateTime!");

      // following line sets the RTC to the date & time this sketch was compiled
      // it will also reset the valid flag internally unless the Rtc device is
      // having an issue

      Rtc1.SetDateTime(compiled);
    }
  }
  
  if (!Rtc1.GetIsRunning())
  {
    if (!wasError("setup GetIsRunning"))
    {
      Serial.println("RTC was not actively running, starting now");
      Rtc1.SetIsRunning(true);
    }
  }

  RtcDateTime now = Rtc1.GetDateTime();
  if (!wasError("setup GetDateTime"))
  {
    if (now < compiled)
    {
      Serial.println("RTC is older than compile time, updating DateTime");
      Rtc1.SetDateTime(compiled);
    }
    else if (now > compiled)
    {
      Serial.println("RTC is newer than compile time, this is expected");
    }
    else if (now == compiled)
    {
      Serial.println("RTC is the same as compile time, while not expected all is still fine");
    }
  }

  // never assume the Rtc was last configured by you, so
  // just clear them to your needed state
  Rtc1.Enable32kHzPin(false);
  wasError("setup Enable32kHzPin");
  Rtc1.SetSquareWavePin(DS3231SquareWavePin_ModeNone); 
  wasError("setup SetSquareWavePin");
  return VALID;
}

void Rtc::begin()
{
  Rtc1.Begin();
  String date_set = convert_to_rtc_date_init_format(this->_date_time.months, this->_date_time.days, this->_date_time.years);
  String time_set = get_current_time_String_format(this->_date_time.hours, this->_date_time.minutes, this->_date_time.seconds);
  RtcDateTime compiled = RtcDateTime(date_set.c_str(), time_set.c_str());
  print_date_time(compiled);
  Serial.println();

  rtc_setup(compiled);

  Serial.println("RTC begins");
}

void Rtc::begin(int hours, int minutes, int seconds, int days, int months, int years)
{
  Rtc1.Begin();
  String date_set = convert_to_rtc_date_init_format(months, days, years);
  String time_set = get_current_time_String_format(hours, minutes, seconds);
  
  RtcDateTime compiled = RtcDateTime(date_set.c_str(), time_set.c_str());
  print_date_time(compiled);
  Serial.println();
  // Rtc1.SetDateTime(compiled);
  rtc_setup(compiled);

  Serial.println("RTC begins");
}

void Rtc::set_time(int hours, int minutes, int seconds, int days, int months, int years)
{
  String date_set = convert_to_rtc_date_init_format(months, days, years);
  String time_set = get_current_time_String_format(hours, minutes, seconds);
  
  RtcDateTime compiled = RtcDateTime(date_set.c_str(), time_set.c_str());
  Rtc1.SetDateTime(compiled);
}

int Rtc::get_seconds()
{
  return Rtc1.GetDateTime().Second();
}

int Rtc::get_minutes()
{
  return Rtc1.GetDateTime().Minute();
}

int Rtc::get_hours()
{
  return Rtc1.GetDateTime().Hour();
}

int Rtc::get_day()
{
  return Rtc1.GetDateTime().Day();
}
int Rtc::get_month()
{
  return Rtc1.GetDateTime().Month();
}
int Rtc::get_year()
{
  return Rtc1.GetDateTime().Year();
}

void Rtc::print_date_time()
{
  RtcDateTime now = Rtc1.GetDateTime();
  if (!wasError("loop GetDateTime"))
  {
      print_date_time(now);
      Serial.println();
  }
}

void Rtc::print_date_time(const RtcDateTime& dt)
{
  char datestring[26];

  snprintf_P(datestring, 
          countof(datestring),
          PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
          dt.Month(),
          dt.Day(),
          dt.Year(),
          dt.Hour(),
          dt.Minute(),
          dt.Second() );
  Serial.print(datestring);
}

// String Rtc::get_data_time(String format)
// {
//   return this->_rtc->getTime(format);
// }
