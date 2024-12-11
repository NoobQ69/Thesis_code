#ifndef IOT_MONITORING_SYSTEM_GPS_H_
#define IOT_MONITORING_SYSTEM_GPS_H_

#define GPS_BAUDRATE 9600
#define GPS_TX_PIN_DEFAULT 33
#define GPS_RX_PIN_DEFAULT 25

#include <TinyGPSPlus.h>
#include <SoftwareSerial.h>

class IOT_GPS
{
  private:
    TinyGPSPlus _gps;
  public:
    IOT_GPS();
    ~IOT_GPS();
    void begin();
    void begin(int tx_pin, int rx_pin, int gps_baud);
    void display_info(bool location, bool date, bool time);
    void read();
};

#endif