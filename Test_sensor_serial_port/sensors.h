/*****************************************************************************
* @file device.h
* device.h header file
*
*                           IoT Lab 2024
*                    ------------------------
*                               CTU
*
* This header file is the definition of IOT_device class.
*
* Contact information:
* <www.>
* <.@gmail.com>
*****************************************************************************/
#ifndef IOT_MONITORING_SYSTEM_SENSORS_H_
#define IOT_MONITORING_SYSTEM_SENSORS_H_

#include "Adafruit_SHT31.h"
#include <Adafruit_BMP085.h>

#define CO2_SENSOR_PIN 4
#define NO2_SENSOR_PIN 2
#define CH4_SENSOR_PIN 15

#define DEBUG_SENSORS 1

typedef struct
{
  float temp;
  float humid;

} SHT30_DATA;


class SHT30
{
  private:
    // Adafruit_SHT31 _sht30;
    SHT30_DATA _sht30_data;
    int _sht30_state;
  public:
    SHT30();
    ~SHT30();
    int begin();
    int read();
    int read(int time_ms, int number_of_sampling);
    float get_data(int type);
};


// class CH4_fermion // CH4
// {
//   private:
//     long long int _fermion_data;
//     int _fermion_state;
//   public:
//     CH4_fermion();
//     ~CH4_fermion();
//     void begin();
//     int read();
//     int read(int time_ms, int number_of_sampling);
//     long long int get_data();
// };

// class MG811 // CO2
// {
//   private:
//     long long int _mg811_data;
//     int _mg811_state;
//   public:
//     MG811();
//     ~MG811();
//     void begin();
//     int read();
//     // int read(int time_ms, int number_of_sampling);
//     long long int get_data();
// };

class CO2
{
  private:
    byte _queryCO2[8] = {0x01, 0x03, 0x00, 0x05, 0x00, 0x01, 0x94, 0x0B};
    byte receiveData[10];

  public:
    int _co2_value;
    CO2();
    // CO2(uint32_t baud_rate, int rxPIN, int txPIN);
    int getSensorValue(byte dataForSend[10], uint8_t sizeOfData, uint8_t possion);
    int getCO2(); // use for get CO2( the unit is ppm)
};

class CH4
{
  private:
    byte _queryCH4[8] = {0x02, 0x03, 0x00, 0x02, 0x00, 0x01, 0x25, 0xF9};
    byte receiveData[10];

  public:
    int _ch4_value; 
    CH4();
    // CH4(int rxPIN, int txPIN);
    // void begin(uint32_t _baud_rate);
    int getSensorValue(byte dataForSend[8], uint8_t sizeOfData, uint8_t possion);
    int getCH4(); // use for get CH4( the unit is ppm)
};

class Rs485_sensors
{
  private:
    CO2 _co2_sensor;
    CH4 _ch4_sensor;
  public:
    Rs485_sensors();
    // Rs485_sensors(int rx, int tx);
    ~Rs485_sensors();
    void begin(unsigned long long int baudrate);
    int read(int type);
    // int read(int time_ms, int number_of_sampling);
    int get_data(int type);
};

class CJMCU_6814 // NO2
{
  private:
    float _cjmcu_6814_data;
    int _cjmcu_6814_state;
  public:
    CJMCU_6814();
    ~CJMCU_6814();
    void begin();
    int read();
    // int read(int time_ms, int number_of_sampling);
    float get_data();
};

class BMP180 // NO2
{
  private:
    Adafruit_BMP085 _bmp;
    long long int _bmp180_data;
    int _bmp180_state;
  public:
    BMP180();
    ~BMP180();
    void begin();
    int read();
    // int read(int time_ms, int number_of_sampling);
    long long int get_data();
};

class IOT_sensors
{
  private:
    SHT30           _sht30_sensor;
    Rs485_sensors   _rs485_sensors;
    CO2 _co2_sensor;
    CH4 _ch4_sensor;
    CJMCU_6814      _no2_sensor;
    BMP180          _pressure_sensor;

  public:
    volatile int _co2_val;
    volatile int _ch4_val;
    IOT_sensors();
    ~IOT_sensors();
    int begin();
    int read_SHT30();
    int read_co2_sensor();
    int read_no2_sensor();
    int read_ch4_sensor();
    // int read_SHT30(int time_ms, int number_of_sampling);
    // int read_co2_sensor(int time_ms, int number_of_sampling);
    // int read_no2_sensor(int time_ms, int number_of_sampling);
    // int read_ch4_sensor(int time_ms, int number_of_sampling);
    int read();
    // int read(int time_ms, int number_of_sampling);
    void print_data_serial();
    int store_to_database();
    float get_data_temp_humid(int type);
    int get_data_co2();
    int get_data_ch4();
    float get_data_no2();
    long long int get_data_pressure();
};

#endif
