#include "device.h"
#include "sensors.h"

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

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
  // pinMode(16, INPUT_PULLUP);
  // db1.begin();
  // IOT_sensors ss1;
  // ss1.begin();
  // ss1.read();
  // ss1.print_data_serial();
  // db1.write_file(SD, "/log.txt","log", true);
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
    // device.handle_buttons_input();
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
