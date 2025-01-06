// #define ESP8266
// #include "LoraMesher.h"
#include "utilities.h"
#include "node.h"
//Using LILYGO TTGO T-BEAM v1.1 
// #define BOARD_LED   2
#define LED_ON      LOW
#define LED_OFF     HIGH
#define RXD2 16
#define TXD2 17

// LoraMesher& radio = LoraMesher::getInstance();

// char Buffer[50];
Node LoRa_node;
//Led flash
// void led_Flash(uint16_t flashes, uint16_t delaymS) {
//     uint16_t index;
//     for (index = 1; index <= flashes; index++) {
//         digitalWrite(BOARD_LED, LED_ON);
//         delay(delaymS);
//         digitalWrite(BOARD_LED, LED_OFF);
//         delay(delaymS);
//     }
// }

/**
 * @brief Function that process the received packets
 *
 */
void processReceivedPackets(void*)
{
  for (;;) 
  {
    LoRa_node.handle_process_received_message();
  }
}

/**
 * @brief Function that process the received packets
 *
 */
void cmd_task(void*)
{
  for (;;) 
  {
    LoRa_node.handle_cmd();
    vTaskDelay(100);
  }
}
/**
 * @brief Function that process the received packets
 *
 */
void serial_task(void*)
{
  for (;;) 
  {
    LoRa_node.handle_command_serial();
    vTaskDelay(100);
  }
}
/**
 * @brief Function that process the received packets
 *
 */
void buttons_task(void*)
{
  for (;;) 
  {
    // LoRa_node.handle_process_received_message();
    vTaskDelay(100);
  }
}
/**
 * @brief Function that process the received packets
 *
 */
void display_task(void*)
{
  for (;;) 
  {
    LoRa_node.handle_get_data_from_display();
    vTaskDelay(100);
  }
}
/**
 * @brief Function that process the received packets
 *
 */
void mqtt_task(void*)
{
  for (;;) 
  {
    LoRa_node.handle_mqtt_communication();
    vTaskDelay(100);
  }
}
/**
 * @brief Function that process the received packets
 *
 */
void actuator_data_msg_task(void*)
{
  for (;;) 
  {
    LoRa_node.handle_data_from_actuator_node();
    vTaskDelay(100);
  }
}

void setup()
{

  // pinMode(BOARD_LED, OUTPUT); //setup pin as output for indicator LED

  Serial.begin(9600);
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
  // pinMode(RXD2, INPUT_PULLUP);
  Serial.println("initBoard");
  LoRa_node.user_recieve_packet_task = processReceivedPackets;
  LoRa_node.cmd_task = cmd_task;
  LoRa_node.serial_task = serial_task;
  LoRa_node.buttons_task = buttons_task;
  LoRa_node.display_task = display_task;
  LoRa_node.mqtt_task = mqtt_task;
  LoRa_node.actuator_data_task = actuator_data_msg_task;
  LoRa_node.begin();

  // led_Flash(2, 125);          //two quick LED flashes to indicate program start
  // setupLoraMesher();
    // int res = xTaskCreate(
    //   processReceivedPackets,
    //   "Receive App Task",
    //   4096,
    //   (void*) 1,
    //   2,
    //   &receiveLoRaMessage_Handle);
}

void loop()
{
  if (LoRa_node._device_role == NODE_SINK)
  {
    LoRa_node.handle_mqtt_communication();
  }
  else
  {
    // LoRa_node.handle_data_from_actuator_node();
    vTaskDelete(NULL);
  }
  // process_cmd_from_serial();
    // for (;;) {
    // }
}