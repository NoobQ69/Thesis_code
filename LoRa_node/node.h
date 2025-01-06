#ifndef LORA_NODE_H__
#define LORA_NODE_H__

#include "FS.h"
#include "SPIFFS.h"
#include <stdint.h>
#include "LoraMesher.h"
#include "database.h"
#include "rtc.h"
#include "display.h"

#define SENSOR_RECORD_FILE_PATH             "/sensor_record.txt"
#define SETTING_FILE_PATH                   "/setting_file.txt"
#define NODE_ROLE_FILE_PATH                 "/node_role.txt"
#define SINK_NODE_ADDRESS_FILE_PATH         "/sink_node_addr.txt"
#define WIFI_FILE_PATH                      "/wifi_st.txt"
#define MQTT_FILE_PATH                      "/mqtt_st.txt"

typedef void(*task_ptr)(void*);

struct dataPacket 
{
  char data_str[75];
};

typedef struct 
{
  int current_data_signal;
  uint8_t sink_addr_available_flag;
  uint8_t wifi_get_params_flag;
  uint8_t mqtt_get_params_flag;

} Node_flags_t;

class Node
{
  private:
      int                   _dsp_current_line_bucket;
    int                   _dsp_current_page_node_addr;
    int                   _sensor_file_num_of_lines;
    uint16_t              _dsp_node_addr[LORA_MAX_NODE_ADDRESS];
    uint16_t              _dsp_current_node_addr;
    uint16_t              _sink_node_addr;
    Node_flags_t          _device_flags;
    char _wifi_ssid[WIFI_NAME_LEN];
    char _wifi_password[WIFI_PASS_LEN];

    char _mqtt_broker[MQTT_BROKER_LEN];
    char _mqtt_port[MQTT_PORT_LEN];
    char _mqtt_username[MQTT_USERNAME_LEN];
    char _mqtt_password[MQTT_PASS_LEN];
    char _mqtt_topic[MQTT_TOPIC_LEN];

    char _cmd_buffer[250];
    char _serial_buffer[250];
    char _LoRa_buffer[100];
    char _display_buffer[250];
    char _buttons_buffer[100];
    char _actuator_buffer[100];
    char _dsp_bucket_buffer[100];
    char _temp_cmd_buffer[100];
    char _mqtt_publish_buffer[250];
    char _data_sensor_buffer[100];
    
    


    Database _database;
    Rtc _rtc;
    Display _display;

    QueueHandle_t        _cmd_msg_queue;
    QueueHandle_t        _serial_msg_queue;
    QueueHandle_t        _LoRa_msg_queue;
    QueueHandle_t        _mqtt_msg_queue;


    dataPacket* helloPacket;
  public:
    int                   _device_role;
    task_ptr user_recieve_packet_task;
    task_ptr cmd_task;
    task_ptr serial_task;
    task_ptr buttons_task;
    task_ptr display_task;
    task_ptr mqtt_task;
    task_ptr actuator_data_task;

    Node();
    void begin();
    void handle_command_serial();
    void handle_get_data_from_display();
    // void handle_mqtt_transceiving();
    void handle_mqtt_communication();
    void handle_data_from_actuator_node();
    int handle_cmd();
    // void process_cmd_from_serial();
    void handle_set_date_time();
    void handle_get_node_role();
    void handle_change_node_role();
    uint16_t handle_get_sink_node_addr();
    void handle_set_sink_node_addr();
    void display_print_date_time();
    void handle_print_data_bucket(int type);

    void handle_get_wifi_params();
    void handle_get_wifi_ssid();
    int connect_to_wifi(const char *ssid, const char *pass);
    void handle_get_mqtt_params();
    void mqtt_setup();
    int publish_to_mqtt_server();
    int mqtt_reconnect();


    void display_print_nodes(int type);
    void get_chosen_node();

    int process_cmd();
    int create_task(TaskFunction_t pv_task_code,
                          const char * name, 
                          const unsigned int stack_size, 
                          void *const param, 
                          UBaseType_t priority, 
                          TaskHandle_t *const task_handler, 
                          const int core);

    void setup_Lora_mesher();
    void print_packet(dataPacket data);
    void print_data_packet(AppPacket<dataPacket>* packet);
    void get_routing_node_address();
    void handle_process_received_message();
};

#endif