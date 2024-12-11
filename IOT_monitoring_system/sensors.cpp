#include "freertos/portmacro.h"
#include "esp32-hal-adc.h"
#include "SoftwareSerial.h"
#include "sensors.h"
#include "utilities.h"
#include <Arduino.h>
#include <Wire.h>
#include "Adafruit_SHT31.h"

uint8_t rx = 15;
uint8_t tx = 2;
SoftwareSerial mySerial(rx, tx);

Adafruit_SHT31 sht31 = Adafruit_SHT31();

IOT_sensors::IOT_sensors()
{
  // this->_sht30_state = SENSOR_DHT30_ERR_NOT_INIT;
}

IOT_sensors::~IOT_sensors()
{

}

int IOT_sensors::begin()
{
  this->_rs485_sensors.begin(SENSOR_RS485_BAUDRATE);
  // this->_co2_sensor.begin();
  // this->_ch4_sensor.begin();
  this->_no2_sensor.begin();
  this->_sht30_sensor.begin();
  this->_pressure_sensor.begin();
  return 0;
}

int IOT_sensors::read()
{
  int state = SENSOR_DHT30_ERR_READ_DATA_FAILED;
  this->_rs485_sensors.read(SENSOR_GET_ALL);
  // state = this->_co2_sensor.read();
  // state = this->_ch4_sensor.read();
  state = this->_no2_sensor.read();
  state = this->_sht30_sensor.read();
  state = this->_pressure_sensor.read();

  return state;
}

// int IOT_sensors::read(int time_ms, int number_of_sampling=5)
// {
//   int state = SENSOR_DHT30_ERR_READ_DATA_FAILED;
//   state = this->_co2_sensor.read(time_ms, number_of_sampling);
//   state = this->_no2_sensor.read(time_ms, number_of_sampling);
//   state = this->_ch4_sensor.read(time_ms, number_of_sampling);
//   state = this->_sht30_sensor.read(time_ms, number_of_sampling);
//   state = this->_pressure_sensor.read(time_ms, number_of_sampling);

//   return state;
// }

void IOT_sensors::print_data_serial()
{
  Serial.println("--- Sensor value ---");
  Serial.print("CO2:"); Serial.println(this->_rs485_sensors.get_data(SENSOR_GET_CO2));
  Serial.print("NO2:"); Serial.println(this->_no2_sensor.get_data());
  Serial.print("CH4:"); Serial.println(this->_rs485_sensors.get_data(SENSOR_GET_CH4));
  Serial.print("Temp:"); Serial.println(this->_sht30_sensor.get_data(0));
  Serial.print("Humid:"); Serial.println(this->_sht30_sensor.get_data(1));
  Serial.print("Pressure:"); Serial.printf("%lld Pa", this->_pressure_sensor.get_data());
  Serial.println();
}

int IOT_sensors::store_to_database()
{
  return 0;
}

float IOT_sensors::get_data_temp_humid(int type)
{
  return this->_sht30_sensor.get_data(type);
}

long long int IOT_sensors::get_data_co2()
{
  return this->_rs485_sensors.get_data(SENSOR_GET_CO2);
}

long long int IOT_sensors::get_data_ch4()
{
  return this->_rs485_sensors.get_data(SENSOR_GET_CH4);
}

long long int IOT_sensors::get_data_no2()
{
  return this->_no2_sensor.get_data();
}

long long int IOT_sensors::get_data_pressure()
{
  return this->_pressure_sensor.get_data();
}

/*
 * FOR SHT30 SENSOR
 */
int IOT_sensors::read_SHT30()
{
  this->_sht30_sensor.read();
}

// int IOT_sensors::read_SHT30(int time_ms = 100, int number_of_sampling = 5)
// {
//   this->_sht30_sensor.read(time_ms, number_of_sampling);
// }

int IOT_sensors::read_co2_sensor()
{
  this->_rs485_sensors.read(SENSOR_GET_CO2); 
  return SENSOR_DHT30_SUCCESS_READ_DATA;
}

// int IOT_sensors::read_co2_sensor(int time_ms = 100, int number_of_sampling = 5)
// {
//   this->_co2_sensor.read(time_ms, number_of_sampling);
// }

int IOT_sensors::read_no2_sensor()
{
  this->_no2_sensor.read();
  return SENSOR_DHT30_SUCCESS_READ_DATA;
}

// int IOT_sensors::read_no2_sensor(int time_ms = 100, int number_of_sampling = 5)
// {
//   this->_no2_sensor.read(time_ms, number_of_sampling);
// }

int IOT_sensors::read_ch4_sensor()
{
  this->_rs485_sensors.read(SENSOR_GET_CH4);
  return SENSOR_DHT30_SUCCESS_READ_DATA;
}

// int IOT_sensors::read_ch4_sensor(int time_ms = 100, int number_of_sampling = 5)
// {
//   this->_ch4_sensor.read(time_ms, number_of_sampling);
// }

/* 
 *  SHT30 sensor 
 */

SHT30::SHT30()
{
  this->_sht30_state = SENSOR_DHT30_ERR_NOT_INIT;
  this->_sht30_data.temp = 0;
  this->_sht30_data.humid = 0;
}

SHT30::~SHT30()
{}

int SHT30::begin()
{

#ifdef DEBUG_SENSORS
  Serial.println("SHT31 test");
#endif

  if (!sht31.begin(0x44))
  {   // Set to 0x45 for alternate i2c addr
#ifdef DEBUG_SENSORS
    Serial.println("Couldn't find SHT31");
#endif
    this->_sht30_state = SENSOR_DHT30_ERR_NOT_FOUND;
  }
  this->_sht30_state = SENSOR_DHT30_SUCCESS_INIT;

  return this->_sht30_state;
}

int SHT30::read()
{
  int state = 0;

  if (this->_sht30_state == SENSOR_DHT30_SUCCESS_INIT)
  {
    this->_sht30_data.temp = sht31.readTemperature();
    this->_sht30_data.humid = sht31.readHumidity();

    if (isnan(this->_sht30_data.temp)) 
    {  // check if 'is not a number'
      this->_sht30_data.temp = 0;
      state = 1;
    }
    
    if (isnan(this->_sht30_data.humid))
    {  // check if 'is not a number'
      this->_sht30_data.humid = 0;
      state += 2;
    }

    if (state == 0)
      return SENSOR_DHT30_SUCCESS_READ_DATA;
    else if(state == 1)
      return SENSOR_DHT30_ERR_READ_TEMP_FAILED;
    else if(state == 2)
      return SENSOR_DHT30_ERR_READ_HUMID_FAILED;
  }

  return SENSOR_DHT30_ERR_READ_DATA_FAILED;
}

// int SHT30::read(int time_ms, int number_of_sampling = 5)
// {
//   int state = 0;
//   long long int temp = 0, humid = 0;
//   Serial.print("Reading temp and humid value");
//   for (int i = 0; i < number_of_sampling; i++)
//   {
//     state = this->read();
//     temp += this->_sht30_data.temp;
//     humid += this->_sht30_data.humid;
//     vTaskDelay(time_ms/portTICK_PERIOD_MS);
//     Serial.print(".");
//   }
//   Serial.println();
//   temp /= number_of_sampling;
//   humid /= number_of_sampling;
//   this->_sht30_data.temp = temp;
//   this->_sht30_data.humid = humid;

//   return state;
// }

float SHT30::get_data(int type = 0)
{
  if (type == 0)
    return this->_sht30_data.temp;
    
  return this->_sht30_data.humid;
}
/* 
 *  Rs485 sensors
 */
// Rs485_sensors::Rs485_sensors(int rx, int tx)
Rs485_sensors::Rs485_sensors()
{

}

Rs485_sensors::~Rs485_sensors()
{

}

void Rs485_sensors::begin(unsigned long long int baudrate)
{
  mySerial.begin(baudrate);
}

int Rs485_sensors::read(int type)
{
  if (type == SENSOR_GET_CO2)
    this->_co2_sensor.getCO2();
  else if (type == SENSOR_GET_CH4)
    this->_ch4_sensor.getCH4();
  else
  {
    this->_co2_sensor.getCO2();
    this->_ch4_sensor.getCH4();
  }

  return 0;
}

// int read(int time_ms, int number_of_sampling);
int Rs485_sensors::get_data(int type)
{
  if (type == SENSOR_GET_CO2)
    return this->_co2_sensor._co2_value;
  else if (type == SENSOR_GET_CH4)
    return this->_ch4_sensor._ch4_value;

  return 0;
}
/* 
 *  CH4 sensor 
 */

CH4::CH4()
{
  this->_ch4_value = 0;
}

int CH4::getSensorValue(byte dataForSend[8], uint8_t sizeOfData, uint8_t possion)
{  
  mySerial.write(dataForSend, 8);

  vTaskDelay(200);

  if (mySerial.available()) 
  {
    mySerial.readBytes(receiveData, sizeOfData - 1);
    return receiveData[possion] << 8 | receiveData[possion + 1];
  }
  else 
  {
    return 0;
  }
}

int CH4::getCH4() 
{
  this->_ch4_value = getSensorValue(_queryCH4, 8, 3);
  return this->_ch4_value;
}

CO2::CO2()
{
  this->_co2_value = 0;
}

int CO2::getSensorValue(byte dataForSend[8], uint8_t sizeOfData, uint8_t possion)
{
  mySerial.write(dataForSend, 8);
  delay(200);
  if (mySerial.available()) {
    mySerial.readBytes(receiveData, sizeOfData - 1);
    return receiveData[possion] << 8 | receiveData[possion + 1];
  } else {
    return 0;
  }
}

int CO2::getCO2()
{
  this->_co2_value = getSensorValue(_queryCO2, 8, 3);
  return _co2_value;
}
// CH4_fermion::CH4_fermion()
// {

// }

// CH4_fermion::~CH4_fermion()
// {}

// void CH4_fermion::begin()
// {
//   return;
// }

// int CH4_fermion::read()
// {
//   // uint16_t temp = map_value(analogRead(CH4_SENSOR_PIN), );
//   this->_fermion_data = analogRead(CH4_SENSOR_PIN);

//   return 0;
// }

// int CH4_fermion::read(int time_ms, int number_of_sampling = 5)
// {
//   long long int ch4 = 0;
//   Serial.print("Reading ch4 value");
//   for (int i = 0; i < number_of_sampling; i++)
//   {
//     this->read();
//     Serial.print(".");
//     ch4 += this->_fermion_data;
//     vTaskDelay(time_ms/portTICK_PERIOD_MS);
//   }
//   Serial.println();
//   ch4 /= number_of_sampling;
//   this->_fermion_data = ch4;
  
//   return 0;
// }

// long long int CH4_fermion::get_data()
// {
//   return this->_fermion_data;
// }

/* 
 *  CO2 sensor 
 */

// MG811::MG811()
// {

// }

// MG811::~MG811()
// {}

// void MG811::begin()
// {
//   return;
// }

// int MG811::read()
// {
//   this->_mg811_data = analogRead(CO2_SENSOR_PIN);
//   return 0;
// }

// int MG811::read(int time_ms, int number_of_sampling = 5)
// {
//   long long int co2 = 0;
//   Serial.print("Reading co2 value");
//   for (int i = 0; i < number_of_sampling; i++)
//   {
//     this->read();
//     co2 += this->_mg811_data;
//     vTaskDelay(time_ms/portTICK_PERIOD_MS);
//     Serial.print(".");
//   }
//   co2 /= number_of_sampling;
//   this->_mg811_data = co2;
//   return 0;
// }

// long long int MG811::get_data()
// {
//   return this->_mg811_data;
// }

/* 
 *  NO2 sensor 
 */

CJMCU_6814::CJMCU_6814()
{

}

CJMCU_6814::~CJMCU_6814()
{}

void CJMCU_6814::begin()
{
  return;
}

int CJMCU_6814::read()
{
  this->_cjmcu_6814_data = 17;
  // this->_cjmcu_6814_data = analogRead(NO2_SENSOR_PIN);
  return 0;
}

// int CJMCU_6814::read(int time_ms, int number_of_sampling = 5)
// {
//   long long int no2 = 0;
//   Serial.print("Reading no2 value");
//   for (int i = 0; i < number_of_sampling; i++)
//   {
//     this->read();
//     no2 += this->_cjmcu_6814_data;
//     vTaskDelay(time_ms/portTICK_PERIOD_MS);
//     Serial.print(".");
//   }
//   Serial.println();
//   no2 /= number_of_sampling;
//   this->_cjmcu_6814_data = no2;
//   return 0;
// }

long long int CJMCU_6814::get_data()
{
  return this->_cjmcu_6814_data;
}

/* 
 *  Pressure sensor 
 */

BMP180::BMP180()
{

}

BMP180::~BMP180()
{}

void BMP180::begin()
{
  this->_bmp180_state = SENSOR_BMP180_NOT_INIT;

  if (!_bmp.begin()) 
  {
	  Serial.println("Could not find a valid BMP085 sensor, check wiring!");
    this->_bmp180_state = SENSOR_BMP180_ERR_INIT_FAILED;
  }
  else
  {
    this->_bmp180_state = SENSOR_BMP180_SUCCESS_INIT;
  }

  return;
}

int BMP180::read()
{
  // Serial.print("Pressure at sealevel (calculated) = ");
  if (this->_bmp180_state == SENSOR_BMP180_SUCCESS_INIT)
  {
    this->_bmp180_data = this->_bmp.readSealevelPressure();
  }
  else
  {
    this->_bmp180_data = 0;
  }
  // Serial.println(" Pa");
  return 0;
}

// int BMP180::read(int time_ms, int number_of_sampling = 5)
// {
//   long long int pressure = 0;
//   Serial.print("Reading pressure value");
//   for (int i = 0; i < number_of_sampling; i++)
//   {
//     this->read();
//     pressure += this->_bmp180_data;
//     vTaskDelay(time_ms/portTICK_PERIOD_MS);
//     Serial.print(".");
//   }
//   Serial.println();
//   pressure /= number_of_sampling;
//   this->_bmp180_data = pressure;
//   return 0;
// }

long long int BMP180::get_data()
{
  return this->_bmp180_data;
}
