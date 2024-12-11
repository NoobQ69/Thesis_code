#include "gps.h"

/*
   This sample sketch demonstrates the normal use of a TinyGPSPlus (TinyGPSPlus) object.
   It requires the use of SoftwareSerial, and assumes that you have a
   4800-baud serial GPS device hooked up on pins 4(rx) and 3(tx).
*/
// static const int RXPin = 25, TXPin = 33;
// static const uint32_t GPSBaud = 9600;

// The TinyGPSPlus object
// TinyGPSPlus gps;

// The serial connection to the GPS device
SoftwareSerial ss(GPS_RX_PIN_DEFAULT, GPS_TX_PIN_DEFAULT);

IOT_GPS::IOT_GPS()
{

}

IOT_GPS:: ~IOT_GPS()
{

}

void IOT_GPS::begin()
{
  ss.begin(GPS_BAUDRATE);

  Serial.println(F("DeviceExample.ino"));
  Serial.println(F("A simple demonstration of TinyGPSPlus with an attached GPS module"));
  Serial.print(F("Testing TinyGPSPlus library v. ")); Serial.println(TinyGPSPlus::libraryVersion());
  Serial.println(F("by Mikal Hart"));
  Serial.println();
}

void IOT_GPS::begin(int tx_pin, int rx_pin, int gps_baud=9600)
{
  return;
}

void IOT_GPS::read()
{
  // This sketch displays information every time a new sentence is correctly encoded.
  while (ss.available() > 0)
    if (this->_gps.encode(ss.read()))
      this->display_info(true, false, false);
  
  // if (millis() > 5000 && gps.charsProcessed() < 10)
  // {
  //   Serial.println(F("No GPS detected: check wiring."));
  //   while(true);
  // }
}

void IOT_GPS::display_info(bool location = true, bool date = false, bool time = false)
{
  if (location == true)
  {
    Serial.print(F("Location: ")); 
    if (this->_gps.location.isValid())
    {
      Serial.print(this->_gps.location.lat(), 6);
      Serial.print(F(","));
      Serial.print(this->_gps.location.lng(), 6);
    }
    else
    {
      Serial.print(F("INVALID"));
    }
  }

  if (date == true)
  {
    Serial.print(F("  Date/Time: "));
    if (this->_gps.date.isValid())
    {
      Serial.print(this->_gps.date.month());
      Serial.print(F("/"));
      Serial.print(this->_gps.date.day());
      Serial.print(F("/"));
      Serial.print(this->_gps.date.year());
    }
    else
    {
      Serial.print(F("INVALID"));
    }
  }

  if (time == true)
  {

    Serial.print(F(" "));
    if (this->_gps.time.isValid())
    {
      if (this->_gps.time.hour() < 10) Serial.print(F("0"));
      Serial.print(this->_gps.time.hour());
      Serial.print(F(":"));
      if (this->_gps.time.minute() < 10) Serial.print(F("0"));
      Serial.print(this->_gps.time.minute());
      Serial.print(F(":"));
      if (this->_gps.time.second() < 10) Serial.print(F("0"));
      Serial.print(this->_gps.time.second());
      Serial.print(F("."));
      if (this->_gps.time.centisecond() < 10) Serial.print(F("0"));
      Serial.print(this->_gps.time.centisecond());
    }
    else
    {
      Serial.print(F("INVALID"));
    }
  }

  Serial.println();
}
