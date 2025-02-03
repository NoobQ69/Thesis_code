#ifndef __LORA_STAR_TOPO_
#define __LORA_STAR_TOPO_
#pragma GCC push_options
#pragma GCC optimize ("O0")

/* ----------------------- PACKET STRUCTURE  ----------------------- */
// (source address)&(destination address)&(type)&(size)&(packet number)&(payload)
/* ---------------------------------------------------------------------------------------------------------------------------------------------*/
/* - source address: indicate the source address of a node */
/* - destination address: indicate the destination address of a node want to send*/
/* - type : indicate the type of the packet whether it is a data packet or routing packet (routing packet is for user node) */
/* - size : the size of the payload in the packet */
/* - packet number : reserved, this field is considered as a feature when the packet is larger than 250 bytes and will develop in the future */
/* - payload : content that a node want to send to other nodes */
// Example: 4444&70E1&1&15&0&abcdefghijklmng
/* ---------------------------------------------------------------------------------------------------------------------------------------------*/

void receive_task(void *params);
void send_task(void *params);
void process_task(void *params);
void process_node_timeout_task(void *params);

#define EVENT_TIME_RANGE        24

#define MAX_PAYLOAD_SIZE        200
#define MAX_PACKET_SIZE         250
#define NODE_MAX_TIMEOUT        3
#define MAX_ADDRESS_NODE        50
#define VALID                   1
#define INVALID                 0

#define PACKET_INDICATOR_NUMBER  5

#define SRC_ADDR_POSITION        0
#define DES_ADDR_POSITION        1
#define TYPE_POSITION            2
#define SIZE_POSITION            3
#define PACKET_NUM_POSITION      4
#define PACKET_PAYLOAD_POSITION  5

// #define NODE_USER                0
// #define NODE_SINK                3

#include <stdint.h>
#include <SPI.h>
#include <LoRa.h>
#include "WiFi.h"
#include "utilities.h"
#include "linked_list.h"

static TaskHandle_t receive_task_handler;
static TaskHandle_t process_packet_task_handler;
static TaskHandle_t process_timeout_task_handler;
static TaskHandle_t send_task_handler;
static TaskHandle_t user_receive_task_handler;

// typedef struct
// {
//   int hours;
//   int minutes;
//   int seconds;
//   int days;
//   int months;
//   int years;

// } DATE_t;

// --------------- FOR LORA STAR TOPOLOGY ---------------

typedef struct
{
  uint16_t addr;                // address of a node
  uint64_t timeout;             // calculate whether the node is online or not
  uint8_t  timeout_counting;    // if timeout number is over NODE_MAX_TIMEOUT (3), the node will be removed

} Node_t;

// typedef struct
// {
//   uint8_t state_transceiving; // imply the state whether is sending or receiving state

// } Lora_star_flags_t;

typedef struct
{
  uint16_t source_addr;
  uint16_t destination_addr;
  uint16_t size;
  uint16_t packet_num;
  uint8_t type;
  char *payload;
  
} LoRa_data_packet_t;

LIST Receive_queue; 
LIST User_receive_queue; 
LIST Send_queue;
static uint16_t local_address;

typedef void (*Task_func_ptr_t)(void *params);
// typedef void (*Func_ptr_callback_t)();

class LoraStar
{
  private:
    // Lora_star_flags_t _flags;
    uint16_t _address_total;
    char _packet_buffer[MAX_PACKET_SIZE];
    char _process_packet_buffer[MAX_PACKET_SIZE];
    char _send_packet_buffer[MAX_PACKET_SIZE];
    SemaphoreHandle_t xSemaphore;

    void _set_in_use();
    void _release_in_use();

  public:
    uint32_t send_errno;
    Node_t _node_array[MAX_ADDRESS_NODE]; // store user nodes
    uint8_t _node_slot[MAX_ADDRESS_NODE]; // indicate whether the element in _node_array is available for storing a node
    Task_func_ptr_t _user_receive_packet_func;
    // Func_ptr_callback_t _user_fnc_callback;
    uint8_t _role;
    LoraStar();
    uint8_t begin();
    uint8_t begin(uint32_t frequency);
    int send_data();
    void receive_data();
    uint8_t check_node_is_stored(uint16_t addr);
    uint8_t add_node(uint16_t addr);
    uint8_t delete_node(uint16_t addr);
    void handle_receive_data_packet();
    void process_timeout();
    // void handle_receive_routing_packet();
    void create_packet_data_and_send(LoRa_data_packet_t *data_packet);
    void create_packet_data_and_send(const char *destination_addr, const char *payload);
    void local_address_init();
    uint16_t get_local_address();
    uint16_t get_param_from_packet(char *packet, int index);
    void print_node_address();
};

LoraStar::LoraStar()
{
  this->xSemaphore = NULL;
  receive_task_handler = NULL;
  send_task_handler = NULL;
  _user_receive_packet_func = NULL;
}

uint8_t LoraStar::begin()
{
  if (!LoRa.begin(433E6)) {
    Serial.println("Starting LoRa failed!");

    return INVALID;
  }
  
  this->xSemaphore = xSemaphoreCreateMutex();
  xTaskCreatePinnedToCore(receive_task, "receive task", 4096, (void *)this, 4, &receive_task_handler, 0);
  xTaskCreatePinnedToCore(send_task, "send task", 4096, (void *)this, 4, &send_task_handler, 0);
  xTaskCreatePinnedToCore(process_task, "process task", 4096, (void *)this, 4, &process_packet_task_handler, 0);
  xTaskCreatePinnedToCore(process_node_timeout_task, "process timeout task", 4096, (void *)this, 4, &process_timeout_task_handler, 0);
  
  if (_user_receive_packet_func != NULL)
    xTaskCreatePinnedToCore(_user_receive_packet_func, "user receive packet task", 4096, NULL, 4, &user_receive_task_handler, 0);

  if (_role == NODE_USER)
  {
    Serial.println("Node role: User node");
    Serial.printf("User node send packet to %x\n", get_local_address());

    LoRa_data_packet_t routing_packet;
    routing_packet.source_addr = get_local_address();
    routing_packet.destination_addr = 0x745C;
    routing_packet.size = 0;
    routing_packet.packet_num = 0;
    routing_packet.type = 2;
    routing_packet.payload = "0";
    create_packet_data_and_send(&routing_packet);
  }
  else
  {
    Serial.println("Node role: Sink node");
  }
  return VALID;
}

uint8_t LoraStar::begin(uint32_t frequency)
{
  if (!LoRa.begin(frequency)) {
    Serial.println("Starting LoRa failed!");

    return INVALID;
  }
  this->xSemaphore = xSemaphoreCreateMutex();

  return VALID;
}

void receive_task(void *params)
{
  LoraStar *lora_obj = (LoraStar*)params;

  while (1)
  {
    lora_obj->receive_data();
    vTaskDelay(2);
  }
}

void send_task(void *params)
{
  LoraStar *lora_obj = (LoraStar*)params;
  BaseType_t TWres = pdPASS;

  while (1)
  {
    Serial.println("Waiting data to send");
    TWres = xTaskNotifyWait(pdTRUE, pdFALSE, NULL, portMAX_DELAY);

    if (TWres == pdPASS)
    {
      while (!Send_queue.is_empty())
      {
        if (!lora_obj->send_data())
        {
          lora_obj->send_errno += 1;
          Serial.printf("Send packet get error: error number = %d\n", lora_obj->send_errno);
        }
        else
        {
          vTaskDelay(5);
        }
      }
      // lora_obj->handle_receive_data_packet();
    }
  }
}

void process_task(void *params)
{
  LoraStar *lora_obj = (LoraStar*)params;
  BaseType_t TWres = pdPASS;
  while (1)
  {
    Serial.println("Waiting data to process");
    TWres = xTaskNotifyWait(
        pdTRUE,
        pdFALSE,
        NULL,
        portMAX_DELAY);
    if (TWres == pdPASS)
    {
      while (!Receive_queue.is_empty())
      {
        // Serial.println("Sending data...");
        lora_obj->handle_receive_data_packet();
        // Serial.println("Sending is done");
      }
    }
  }
}

#define NODE_MAX_DELAY_TIMEOUT 60000

void process_node_timeout_task(void *params)
{
  LoraStar *lora_obj = (LoraStar*)params;

  while (1)
  {
    lora_obj->process_timeout();
    vTaskDelay(NODE_MAX_DELAY_TIMEOUT);
  }
}

#define MAX_TIMEOUT 60000

void LoraStar::process_timeout()
{
  if (_role == NODE_SINK)
  {
    print_node_address();
    Serial.println("Recalculate timeout...");
    for (int i = 0; i < MAX_ADDRESS_NODE; i++)
    {
      if (_node_slot[i] == 1)
      {
        if (millis() - _node_array[i].timeout > MAX_TIMEOUT)
        {
          // this->_node_slot[i] = 0;
          _node_array[i].timeout_counting += 1;
          Serial.printf("Node %x not update timeout in %d time(s)\n", _node_array[i].addr, _node_array[i].timeout_counting);
          if (_node_array[i].timeout_counting > 3)
          {
            Serial.printf("Node timeout: %x, deleted\n", _node_array[i].addr);
            delete_node(_node_array[i].addr);
          }
        }
      }
    }
  }
  else
  {
    LoRa_data_packet_t routing_packet;
    routing_packet.source_addr = get_local_address();
    routing_packet.destination_addr = 0x745C;
    routing_packet.size = 0;
    routing_packet.packet_num = 0;
    routing_packet.type = 2;
    routing_packet.payload = "0";
    create_packet_data_and_send(&routing_packet);
  }
}

int LoraStar::send_data()
{
  Serial.println("Send task: Sending packet...");
  LoRa.beginPacket();
  LoRa.print(_send_packet_buffer);
  if (LoRa.endPacket() == 0)
  {
    Send_queue.delete_head();
    return 0;
  }
  Serial.println("Send task: Packet sent.");

  Send_queue.delete_head();
  return 1;
}

void LoraStar::local_address_init() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  local_address = (mac[4] << 8) | mac[5];
}

uint16_t LoraStar::get_local_address() {
  if (local_address == 0)
    local_address_init();
  return local_address;
}

uint16_t LoraStar::get_param_from_packet(char *packet, int index)
{
  uint16_t num;
  char temp_str[10];
  memset(temp_str, 0, 10);
  split_string_char(packet, '&', index, temp_str);
  if (index == SRC_ADDR_POSITION || index == DES_ADDR_POSITION)
  {
    num = strtol(temp_str, NULL, 16);
  }
  else
  {
    num = strtol(temp_str, NULL, 10);
  }
  return num;
}

void LoraStar::receive_data()
{
  BaseType_t TWres = pdPASS;
  int i = 0;
  int packetSize = LoRa.parsePacket();

  if (packetSize) 
  {
    // received a packet
    Serial.print("Received packet :");

    memset(this->_packet_buffer, 0, MAX_PACKET_SIZE);
    // read packet
    while (LoRa.available())
    {
      this->_packet_buffer[i] = (char)LoRa.read();
      i++;
    }
    Serial.print(this->_packet_buffer);
    // print RSSI of packet
    Serial.print(" with RSSI ");
    Serial.println(LoRa.packetRssi());
    
    if (count_character(this->_packet_buffer, '&') == PACKET_INDICATOR_NUMBER)
    {
      if (get_param_from_packet(this->_packet_buffer, DES_ADDR_POSITION) == get_local_address())
      {
        Serial.println("Packet is sent for me.");
        Receive_queue.add_tail(this->_packet_buffer);
        TWres = xTaskNotifyFromISR(
                    process_packet_task_handler,
                    0,
                    eSetValueWithoutOverwrite,
                    &TWres);
      }
      else
      {
        Serial.println("Packet is sent not for me, discard.");
      }
    }
    else
    {
      Serial.println("Wrong packet format, discard this data.");
    }
  }
}

uint8_t LoraStar::check_node_is_stored(uint16_t addr)
{
  for (int i = 0; i < MAX_ADDRESS_NODE; i++)
  {
      if (this->_node_array[i].addr == addr)
      {
        Serial.printf("Node is already stored, update timeout");
        this->_node_array[i].timeout = millis();
        this->_node_array[i].timeout_counting = 0;
        
        return 1;
      }
  }
  return 0;
}

uint8_t LoraStar::add_node(uint16_t addr)
{
  for (int i = 0; i < MAX_ADDRESS_NODE; i++)
  {
    if (this->_node_slot[i] == 0)
    {
      this->_node_array[i].addr = addr;
      this->_node_array[i].timeout = millis();
      this->_node_array[i].timeout_counting = 0;
      this->_node_slot[i] = 1;
      Serial.printf("Stored node %x\n", addr);

      return 0;
    }
  }
  return 1;
}

uint8_t LoraStar::delete_node(uint16_t addr)
{
  for (int i = 0; i < MAX_ADDRESS_NODE; i++)
  {
    if (this->_node_array[i].addr == addr)
    {
      this->_node_array[i].addr = 0;
      this->_node_slot[i] = 0;
      this->_node_array[i].timeout_counting = 0;
      return 0;
    }
  }
  return 1;
}

void LoraStar::handle_receive_data_packet()
{
  // get packet from receive queue
  BaseType_t TWres = pdPASS;
  memset(this->_process_packet_buffer, 0, MAX_PACKET_SIZE);

  Receive_queue.get_payload_head(this->_process_packet_buffer);
  Receive_queue.delete_head();

  if (strlen(this->_process_packet_buffer))
  {
    if (get_param_from_packet(this->_process_packet_buffer,TYPE_POSITION) == 1) // packet is data packet, add to user_receive_queue
    {
      Serial.printf("A data packet received :%s, add to user queue\n",this->_process_packet_buffer);

      User_receive_queue.add_tail(this->_process_packet_buffer);
      if (_user_receive_packet_func != NULL)
      {
        TWres = xTaskNotifyFromISR(user_receive_task_handler, 0, eSetValueWithoutOverwrite, &TWres);
      }
    }
    else // packet is routing packet, add node to node_array
    {
      if (_role == NODE_SINK)
      {
        Serial.println("A routing packet received.");
        int addr = get_param_from_packet(this->_process_packet_buffer, SRC_ADDR_POSITION);
        // check if this node is alreary stored
        if (!check_node_is_stored(addr))
        {
          Serial.println("New node, stored to array.");
          if (add_node(addr) == 1)
          {
            Serial.printf("Cannot store more nodes! Maximum node can be store is %d\n",MAX_ADDRESS_NODE);
          }
        }
      }
      // add to node array
    }
  }
}

void LoraStar::create_packet_data_and_send(const char *destination_addr, const char *payload)
{
  BaseType_t TWres = pdPASS;
  // LoRa_data_packet_t data_packet
  Serial.printf("Packet content: %s\n", payload);
  sprintf(_send_packet_buffer,"%x&%s&%d&%d&%d&%s",
          get_local_address(),
          destination_addr,
          1,
          strlen(payload),
          0,
          payload);
  
  _set_in_use();
  Send_queue.add_tail(_send_packet_buffer);
  _release_in_use();
  
  TWres = xTaskNotifyFromISR(send_task_handler, 0, eSetValueWithoutOverwrite, &TWres);
}

void LoraStar::create_packet_data_and_send(LoRa_data_packet_t *data_packet)
{
  BaseType_t TWres = pdPASS;
  Serial.printf("Packet content: %s\n",data_packet->payload);
  sprintf(_send_packet_buffer,"%x&%x&%d&%d&%d&%s",
          data_packet->source_addr,
          data_packet->destination_addr,
          data_packet->type,
          data_packet->size,
          data_packet->packet_num,
          data_packet->payload);
  
  _set_in_use();
  Send_queue.add_tail(_send_packet_buffer);
  _release_in_use();
  
  TWres = xTaskNotifyFromISR(send_task_handler, 0, eSetValueWithoutOverwrite, &TWres);
}

void LoraStar::print_node_address()
{
  Serial.println("Current node in array:");
  for (int i = 0; i < MAX_ADDRESS_NODE; i++)
  {
    if (this->_node_slot[i])
    {
      Serial.printf("Node %x with timeout count to %d\n", this->_node_array[i].addr, this->_node_array[i].timeout_counting);
    }
  }
}

// template <class T>
void LoraStar::_set_in_use() {
  while (xSemaphoreTake(this->xSemaphore, (TickType_t) 10) != pdTRUE) {
    ESP_LOGW(LM_TAG, "List in Use Alert");
  }
}

// template <class T>
void LoraStar::_release_in_use() {
  xSemaphoreGive(this->xSemaphore);
}

#endif