#pragma GCC push_options
#pragma GCC optimize ("O0")
#include "SoftwareSerial.h"

byte __queryCO2[8] = {0x01, 0x03, 0x00, 0x05, 0x00, 0x01, 0x94, 0x0B};
byte __queryCH4[8] = {0x02, 0x03, 0x00, 0x02, 0x00, 0x01, 0x25, 0xF9};
uint32_t __baud_rate = 9600;
byte _receiveData[10];
// SoftwareSerial mySerial(rx, tx);
SoftwareSerial _mySerial(25, 26);

int _getSensorValue(byte dataForSend[8], uint8_t sizeOfData, uint8_t possion)
{
  Serial.println("-------Main loop----------");
  _mySerial.write(dataForSend, 8);
  delay(200);
  if (_mySerial.available()) 
  {
    _mySerial.readBytes(_receiveData, sizeOfData - 1);
    for (int i =0; i < 10; i++)
    {
      Serial.print(_receiveData[i]);
      Serial.print(" ");
    }
    Serial.println();
    int val = (int)(_receiveData[possion] << 8 | _receiveData[possion + 1]);
    return val;
    Serial.println("-----------------");
    return (int)(_receiveData[possion] << 8 | _receiveData[possion + 1]);
  } else {
    return 0;
  }
}

#include "device.h"
// #include "sensors.h"

static const BaseType_t App_cpu     = 0;
TaskHandle_t input_task_handler   = NULL;
TaskHandle_t handle_cmd_task_handler   = NULL;
// TaskHandle_t buttons_task_handler   = NULL;
// TaskHandle_t serial_task_handler    = NULL;
TaskHandle_t store_db_task_handler  = NULL;
// TaskHandle_t btn_task_handler       = NULL;
TaskHandle_t LoRa_transceiver_task_handler       = NULL;

IOT_device device;
// IOT_database db1;
#define RXD2 16
#define TXD2 17

// IOT_sensors ss1;
// CO2 co2_ss1;
// CH4 ch4_ss1;

void callback()
{
  _mySerial.begin(9600);
  device._sensors._co2_val = _getSensorValue(__queryCO2, 8, 3);
  _mySerial.begin(4800);
  device._sensors._ch4_val = _getSensorValue(__queryCH4, 8, 3);
}

void setup() {
  // put your setup code here, to run once:
  _mySerial.begin(9600);
  Serial.begin(9600);
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
  device.callbackfnc = callback;
  device.begin();
  device.create_task(input_task, "Input task", 8192, NULL,   4, &input_task_handler, App_cpu);
  device.create_task(store_data_to_database_task, "Store db task", 8192, NULL,  4, &store_db_task_handler, App_cpu);
  device.create_task(LoRa_transceiver_task, "Serial 2 task", 4096, NULL,  4, &LoRa_transceiver_task_handler, App_cpu);
  device.create_task(handle_commandline_task, "H C task", 8192, NULL, 4, &handle_cmd_task_handler, App_cpu);
  Serial.println("Done");
  // device.create_task(buttons_task,                "Btn task", 4096, NULL,  4, &btn_task_handler, App_cpu);
}

void handle_commandline_task(void *param)
{
  while(1)
  {
    // do something
    device.handle_cmd();
    // vTaskDelay(1000);
  }
}

void LoRa_transceiver_task(void *param)
{
  while (1)
  {
    device.handle_data_from_transceiver();
    vTaskDelay(200);
  }
}

void input_task(void *parameter)
{
  // String data_lcd = "";

  for(;;)
  {
    device.handle_buttons_input();
    device.handle_get_data_from_display();
    device.handle_command_serial();
    // Serial.println("Pass...");
    vTaskDelay(100);
  }
}

void store_data_to_database_task(void *parameter)
{
  for(;;)
  {
    device.handle_save_to_database();
    vTaskDelay(500);
  }
}

void loop()
{
  device.run();
}
