#include "WiFi.h"
#include <PubSubClient.h>
#include "SPI.h"
#include "node.h"
#include "spiffs_utilities.h"

const char *Default_ssid = "Sussy_hotspot";
const char *Default_password = "nguyenqu@ng10";

const char *Default_mqtt_broker = "dev.iotlab.net.vn";
//const char *mqtt_broker = "broker.mqtt.cool" ;
const int Default_mqtt_port = 1883;
const char *Default_mqtt_username = "api1@Iotlab";
const char *Default_mqtt_password = "Iotlab@2023";
const char *Default_topic_pub_sensor = "data/view";

TaskHandle_t receiveLoRaMessage_Handle = NULL;
TaskHandle_t cmd_task_handler          = NULL;
TaskHandle_t serial_task_handler       = NULL;
TaskHandle_t buttons_task_handler      = NULL;
TaskHandle_t display_task_handler      = NULL;
TaskHandle_t actuator_task_handler     = NULL;
TaskHandle_t mqtt_task_handler         = NULL;

// char Dsp_node_addr_label[][10] = {"b1.txt=\"","b2.txt=\"","b3.txt=\"","b4.txt=\"","b5.txt=\"","b6.txt=\""};
// char Mqtt_json_str_format[] = {"{ \"sensorRecords\": [{\"dataStreamId\": \"%d\",\"result\": \"%s %s\"}] }"};
char Event_time_string_temp[20];

SPIClass newSPI(VSPI);
LoraMesher& Radio = LoraMesher::getInstance();

WiFiClient espClient;
PubSubClient client(espClient);
/**
 * @brief ...
 *
 */
Node::Node()
{
  this->_device_role                              = NODE_SINK;
  this->_device_flags.current_data_signal         = NO_DATA_SIGNAL;
  this->_device_flags.sink_addr_available_flag         = DISABLE;
  this->_device_flags.mqtt_get_params_flag         = FAILED;
  this->_device_flags.wifi_get_params_flag         = FAILED;

  this->_cmd_msg_queue                            = xQueueCreate(10, sizeof(int));
  this->_serial_msg_queue                         = xQueueCreate(2, sizeof(int));
  this->_LoRa_msg_queue                           = xQueueCreate(2, sizeof(int));
  this->_mqtt_msg_queue                           = xQueueCreate(2, sizeof(int));
}
/**
 * @brief ...
 *
 */
void Node::begin()
{
  char node_role_str[2];
  char sink_node_str[6];
  memset(sink_node_str, 0, 6);
  int line;
  if (initSPIFFS() == 1)
  {
    if (readFile(SPIFFS, NODE_ROLE_FILE_PATH, node_role_str) == SUCCESS)
    {
      this->_device_role = convert_string_to_int(node_role_str, 1);
    }
    else
    {
      this->_device_role = NODE_USER;
    }

    if (readFile(SPIFFS, SINK_NODE_ADDRESS_FILE_PATH, sink_node_str) == SUCCESS)
    {
      this->_sink_node_addr = strtol(sink_node_str, NULL, 16);
      this->_device_flags.sink_addr_available_flag = ENABLE;
    }
  }
  // this->_device_role = NODE_SINK;
  
  this->helloPacket = new dataPacket;
  this->setup_Lora_mesher();

  if (this->_device_role == NODE_SINK)
  {
    this->_rtc.begin();
    if (this->_database.begin() == DATABASE_SUCCESS_INIT)
    {
      this->_device_flags.wifi_get_params_flag = SUCCESS;
      this->_device_flags.mqtt_get_params_flag = SUCCESS;
    }
    this->_display.begin();
    this->create_task(buttons_task, "buttons task", 8192, NULL,   4, &buttons_task_handler, 0);
    this->create_task(display_task, "display task", 8192, NULL,   4, &display_task_handler, 0);

    if (this->_device_flags.wifi_get_params_flag == FAILED)
    {
      memset(this->_wifi_ssid, 0, WIFI_NAME_LEN);
      strcpy(this->_wifi_ssid, Default_ssid);
      memset(this->_wifi_password, 0, WIFI_PASS_LEN);
      strcpy(this->_wifi_password, Default_password);
    }
    else
    {
      this->_database.read_file(SD, WIFI_FILE_PATH, _cmd_buffer,&line);
      memset(this->_wifi_ssid, 0, WIFI_NAME_LEN);
      memset(this->_wifi_password, 0, WIFI_PASS_LEN);
      split_string_char(this->_cmd_buffer, ',',1,this->_wifi_ssid);
      split_string_char(this->_cmd_buffer, ',',2,this->_wifi_password);
    }
    if (this->_device_flags.wifi_get_params_flag == FAILED)
    {
      memset(this->_mqtt_broker, 0, MQTT_BROKER_LEN);
      strcpy(this->_mqtt_broker, Default_mqtt_broker);
      memset(this->_mqtt_port, 0, MQTT_PORT_LEN);
      strcpy(this->_mqtt_port, String(Default_mqtt_port).c_str());
      memset(this->_mqtt_username, 0, MQTT_USERNAME_LEN);
      strcpy(this->_mqtt_username, Default_mqtt_username);
      memset(this->_mqtt_password, 0, MQTT_PASS_LEN);
      strcpy(this->_mqtt_password, Default_mqtt_password);
      memset(this->_mqtt_topic, 0, MQTT_TOPIC_LEN);
      strcpy(this->_mqtt_topic, Default_topic_pub_sensor);
    }
    else
    {
      this->_database.read_file(SD, MQTT_FILE_PATH, _cmd_buffer, &line);
      memset(this->_mqtt_broker, 0, MQTT_BROKER_LEN);
      memset(this->_mqtt_port, 0, MQTT_PORT_LEN);
      memset(this->_mqtt_username, 0, MQTT_USERNAME_LEN);
      memset(this->_mqtt_password, 0, MQTT_PASS_LEN);
      memset(this->_mqtt_topic, 0, MQTT_TOPIC_LEN);
      split_string_char(this->_cmd_buffer, ',',1,this->_mqtt_broker);
      split_string_char(this->_cmd_buffer, ',',2,this->_mqtt_port);
      split_string_char(this->_cmd_buffer, ',',3,this->_mqtt_username);
      split_string_char(this->_cmd_buffer, ',',4,this->_mqtt_password);
      split_string_char(this->_cmd_buffer, ',',5,this->_mqtt_topic);
    }
    Serial.println(this->_mqtt_broker);
    Serial.println(this->_mqtt_port);
    Serial.println(this->_mqtt_username);
    Serial.println(this->_mqtt_password);
    Serial.println(this->_mqtt_topic);
    // this->create_task(mqtt_task, "mqtt task", 8192, NULL,   4, &mqtt_task_handler, 0);
  }
  else if (this->_device_role == NODE_USER)
  {
    this->_rtc.begin();
    this->create_task(actuator_data_task, "actuator task", 8192, NULL,   4, &actuator_task_handler, 0);
  }
  // else if (this->_device_role == NODE_GATEWAY || this->_device_role == NODE_USER)
  // {
  pinMode(13, OUTPUT);
  pinMode(12, OUTPUT);
  digitalWrite(13,LOW);
  digitalWrite(12,LOW);
  // }
  this->create_task(cmd_task, "cmd task", 8192, NULL,   4, &cmd_task_handler, 0);
  this->create_task(serial_task, "serial task", 8192, NULL,   4, &serial_task_handler, 0);

  Serial.println(_wifi_ssid);
  Serial.println(_wifi_password);
  Serial.println(_mqtt_broker);
  Serial.println(_mqtt_port);
  Serial.println(_mqtt_username);
  Serial.println(_mqtt_password);
  Serial.println(_mqtt_topic);
  if (this->_device_role == NODE_SINK)
  {
    this->mqtt_setup();
    this->connect_to_wifi(this->_wifi_ssid, this->_wifi_password);
  }
  // this->create_task(mqtt_task, "mqtt task", 8192, NULL,   4, &mqtt_task_handler, 0);
}
/**
 * @brief ...
 *
 */
void Node::handle_command_serial()
{
  if (Serial.available())
  {
    vTaskDelay(20);
    char c;
    int i = 0;
    memset(this->_serial_buffer, 0, SETTING_MAX_BUFFER);
    // String command = "";
    while (Serial.available())
    {
      c = Serial.read();
      if (c == '\n') break;
      this->_serial_buffer[i] = c;
      i++;
    }

    int msg = SERIAL_DATA_SIGNAL;
    if (xQueueSend(this->_cmd_msg_queue, (void *)&msg, 100) != pdTRUE)
    {
      Serial.println("From Serial: CMD event queue is full");
    }
    // handle_cmd();
  }
}
/**
 * @brief ...
 *
 */
void Node::handle_get_data_from_display()
{
  if (this->_display.get_data_from_display(this->_display_buffer) > 0)
  {
    int msg = DISPLAY_DATA_SIGNAL;
    
    if (xQueueSend(this->_cmd_msg_queue, (void *)&msg, 100) != pdTRUE)
    {
      Serial.println("From display: CMD event queue is full");
    }
    // this->handle_cmd();
  }
}

// void Node::process_cmd_from_serial()
// {
//   int i = 0;
//   char c;

//   if (Serial.available())
//   {
//     while (Serial.available())
//     {
//       c = Serial.read();
//       this->_serial_buffer[i] = c;
//       i++;
//     }

//     // send msg to cmd task to notify the task receiving data
//     int msg = LORA_NODE_DATA_SIGNAL;
//     if (xQueueSend(this->_cmd_msg_queue, (void *)&msg, 100) != pdTRUE)
//     {
//       Serial.println("From serial: CMD event queue is full");
//     }
//   }
// }

/**
 * @brief ...
 *
 */
void Node::handle_get_node_role()
{
  if (this->_device_flags.current_data_signal == DISPLAY_DATA_SIGNAL)
  {
    if (this->_device_role == NODE_ACTUATOR_DEVICE || this->_device_role == NODE_ACTUATOR_STATION)
    {
      this->_display.send_data_to_display("notify_label.txt=\"Cannot access nodes from node actuator!\"");
    }
    else if (this->_device_role == NODE_SINK)
    {
      this->_display.send_data_to_display("page fn_page");
    }
  }
  else if (this->_device_flags.current_data_signal == SERIAL_DATA_SIGNAL)
  {
    Serial.printf("DEVICE_ROLE %d\n",this->_device_role);
  }
}
/**
 * @brief ...
 *
 */
void Node::handle_change_node_role()
{
  int pos = get_index_from_string(this->_cmd_buffer, ',', 0);
  int role_number = convert_string_to_int(this->_cmd_buffer + (pos+1), 1);
  
  if (role_number >= 0 && role_number < 5)
  {
    writeFile(SPIFFS, NODE_ROLE_FILE_PATH, this->_cmd_buffer + (pos+1));
    ESP.restart();
  }
  else
  {
    if (this->_device_flags.current_data_signal == SERIAL_DATA_SIGNAL);
      Serial.println("Role is invalid!");
    
  }
}
/**
 * @brief ...
 *
 */
uint16_t Node::handle_get_sink_node_addr()
{
  if (this->_device_flags.current_data_signal == SERIAL_DATA_SIGNAL)
  {
    Serial.printf("Sink node: %x",this->_sink_node_addr);
  }
  return this->_sink_node_addr;
}
/**
 * @brief ...
 *
 */
void Node::handle_set_sink_node_addr()
{
  int pos = get_index_from_string(this->_cmd_buffer, ',', 0) + 1;
  this->_sink_node_addr = strtol(this->_cmd_buffer + pos, NULL, 16);
  writeFile(SPIFFS, SINK_NODE_ADDRESS_FILE_PATH, this->_cmd_buffer + pos);

  if (this->_device_flags.current_data_signal == SERIAL_DATA_SIGNAL)
  {
    Serial.printf("Sink node: %x",this->_sink_node_addr);
  }
}
/**
 * @brief ...
 *
 */
void Node::handle_set_date_time()
{
  int date_time_arr[] = {0, 0, 0, 1, 1, 2024};
  char time_buffer[5];
  
  for (int i = 0; i < 6; i++)
  {
    memset(time_buffer, 0, 5);
    split_string_char(this->_cmd_buffer, ',', i+1, time_buffer);
    date_time_arr[i] = convert_string_to_int(time_buffer, strlen(time_buffer));
  }
  this->_rtc.set_time(date_time_arr[0], date_time_arr[1], date_time_arr[2], date_time_arr[3], date_time_arr[4], date_time_arr[5]);
}
/**
 * @brief ...
 *
 */
void Node::display_print_date_time()
{
  // char temp[3];

  memset(Event_time_string_temp, 0, 20);
  sprintf(Event_time_string_temp, "t14.txt=\"%d:%d\"",this->_rtc.get_hours(), this->_rtc.get_minutes());
  this->_display.send_data_to_display(Event_time_string_temp);
}
/**
 * @brief ...
 *
 */
void Node::handle_print_data_bucket(int type)
{
  if (this->_device_role == NODE_GATEWAY || this->_device_role == NODE_USER)
  {
    this->_display.send_data_to_display("page note_page");
    this->_display.send_data_to_display("notify_label.txt=\"Monitor only for sink and actuator nodes!\"");
    return;
  }

  if (type == DSP_ENTER)
  {
    this->_database.get_number_of_line(SD, SENSOR_RECORD_FILE_PATH, &this->_sensor_file_num_of_lines);
  }
  else if (type == DSP_UP)
  {
    if (_dsp_current_line_bucket - 5 > 0) _dsp_current_line_bucket -= 5;
  }
  else if (type == DSP_DOWN)
  {
    if (_dsp_current_line_bucket + 5 < this->_sensor_file_num_of_lines) _dsp_current_line_bucket += 5;
  }

  memset(this->_cmd_buffer, 0, SERIAL_INPUT_MAX_BUFFER);
  memset(this->_dsp_bucket_buffer, 0, 50);

  this->_database.read_line(SD, SENSOR_RECORD_FILE_PATH, this->_dsp_bucket_buffer, _dsp_current_line_bucket);
  sprintf(this->_cmd_buffer,"t1.txt=\"%s\\r\"", this->_dsp_bucket_buffer);
  this->_display.send_data_to_display(this->_cmd_buffer);
  
  for (int i = 1; i < 5; i++)
  {
    memset(this->_cmd_buffer, 0, SERIAL_INPUT_MAX_BUFFER);
    memset(this->_dsp_bucket_buffer, 0, 50);
    // strcat(this->_cmd_buffer,"t1.txt+=\"");
    this->_database.read_line(SD, SENSOR_RECORD_FILE_PATH, this->_dsp_bucket_buffer, _dsp_current_line_bucket+i);
    sprintf(this->_cmd_buffer,"t1.txt+=\"%s\\r\"", this->_dsp_bucket_buffer);
    this->_display.send_data_to_display(this->_cmd_buffer);
  }
}
/**
 * @brief ...
 *
 */
void Node::handle_get_wifi_params()
{
  memset(this->_wifi_ssid,0,WIFI_NAME_LEN);
  split_string_char(this->_cmd_buffer,',',1,this->_wifi_ssid);
  memset(this->_wifi_password,0,WIFI_NAME_LEN);
  split_string_char(this->_cmd_buffer,',',2,this->_wifi_password);

  memset(this->_temp_cmd_buffer,0,WIFI_NAME_LEN);
  sprintf(this->_temp_cmd_buffer,"wifi_st_lb.txt=\"Connected to:%s\"", this->_wifi_ssid);
  // do something else
  if (this->_device_flags.current_data_signal == DISPLAY_DATA_SIGNAL)
  {
    this->_display.send_data_to_display(this->_temp_cmd_buffer);
  }
  else if (this->_device_flags.current_data_signal == SERIAL_DATA_SIGNAL)
  {
    Serial.println("Set wifi params done");
  }

  this->_database.write_file(SD, WIFI_FILE_PATH, this->_cmd_buffer, false);
}
/**
 * @brief ...
 *
 */
int Node::connect_to_wifi(const char *ssid, const char *pass)
{
  Serial.println("Connecting to WiFi..");

  WiFi.begin(ssid, pass);
  
  while (WiFi.status() != WL_CONNECTED) 
  {
    vTaskDelay(500);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");
  return 0;
}
/**
 * @brief ...
 *
 */
static void check_and_store_string(char *temp_str, char *str)
{
  if (strlen(temp_str) > 0)
  {
    memset(str, 0, strlen(str));
    strcpy(str, temp_str);
  }
}
/**
 * @brief ...
 *
 */

void Node::handle_get_mqtt_params()
{
  memset(this->_temp_cmd_buffer, 0, 50);
  split_string_char(this->_cmd_buffer,',',1,this->_temp_cmd_buffer);
  check_and_store_string(this->_temp_cmd_buffer, this->_mqtt_broker);

  memset(this->_temp_cmd_buffer,0,50);
  split_string_char(this->_cmd_buffer,',',2,this->_temp_cmd_buffer);
  check_and_store_string(this->_temp_cmd_buffer, this->_mqtt_port);

  memset(this->_temp_cmd_buffer,0,50);
  split_string_char(this->_cmd_buffer,',',3,this->_temp_cmd_buffer);
  check_and_store_string(this->_temp_cmd_buffer, this->_mqtt_username);

  memset(this->_temp_cmd_buffer,0,50);
  split_string_char(this->_cmd_buffer,',',4,this->_temp_cmd_buffer);
  check_and_store_string(this->_temp_cmd_buffer, this->_mqtt_password);

  memset(this->_temp_cmd_buffer,0,50);
  split_string_char(this->_cmd_buffer,',',5,this->_temp_cmd_buffer);
  check_and_store_string(this->_temp_cmd_buffer, this->_mqtt_topic);

  // do something else
  if (this->_device_flags.current_data_signal == DISPLAY_DATA_SIGNAL)
  {
    this->_display.send_data_to_display("st_label.txt=\"OK\"");
  }
  else if (this->_device_flags.current_data_signal == SERIAL_DATA_SIGNAL)
  {
    Serial.println("Set mqtt params done");
  }

  this->_database.write_file(SD, MQTT_FILE_PATH, this->_cmd_buffer, false);
}
/**
 * @brief ...
 *
 */

static void callback(char *topic, byte *message, unsigned int length)
{
  Serial.print("Message arrived on topic: ");
  Serial.println(topic);
  Serial.print("Message: ");
  // String messageTemp;
  for (int i = 0; i < length; i++) {
    // messageTemp += (char)message[i];
  }
  // Serial.println(messageTemp);
//  XU LY DU LIEU
}
/**
 * @brief ...
 *
 */
void Node::mqtt_setup()
{
  client.setServer(this->_mqtt_broker, Default_mqtt_port);

  client.setCallback(callback);
}
/**
 * @brief ...
 *
 */
int Node::publish_to_mqtt_server()
{
  if (client.publish(Default_topic_pub_sensor, this->_mqtt_publish_buffer)) 
  {
    Serial.println("Data sending successfully!");
    // json_publish [256] = NULL;
    return SUCCESS;
  }
  
  Serial.println("Failed to send data!");
  return FAILED;
}
/**
 * @brief ...
 *
 */
int Node::mqtt_reconnect()
 {
  while(!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("esp8266-client" , this->_mqtt_username, this->_mqtt_password)) 
    {
      Serial.println("Connected");
    } 
    else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      vTaskDelay(5000);
    }
  }
  return 0;
}
/**
 * @brief ...
 *
 */

char mqtt_temp_buff[15];

void Node::handle_mqtt_communication()
{
  int item;
  if (xQueueReceive(this->_mqtt_msg_queue, (void *)&item, 0) == pdTRUE)
  {
    // do something
    if (item == SEND_SENSOR_DATA_SIGNAL)
    {
      memset(this->_mqtt_publish_buffer,0, strlen(this->_mqtt_publish_buffer));
      memset(mqtt_temp_buff, 0, 15);
      split_string_char(this->_data_sensor_buffer, ',', 1, mqtt_temp_buff);
      sprintf(this->_mqtt_publish_buffer,"{ \"sensorRecords\": [{\"dataStreamId\": \"%d\",\"result\": \"%s %s\"}] }", id_CO2, mqtt_temp_buff,"ppm");
      this->publish_to_mqtt_server();

      memset(this->_mqtt_publish_buffer,0, strlen(this->_mqtt_publish_buffer));
      memset(mqtt_temp_buff, 0, 15);
      split_string_char(this->_data_sensor_buffer, ',', 2, mqtt_temp_buff);
      sprintf(this->_mqtt_publish_buffer,"{ \"sensorRecords\": [{\"dataStreamId\": \"%d\",\"result\": \"%s %s\"}] }", id_CH4, mqtt_temp_buff,"ppm");
      this->publish_to_mqtt_server();
      
      memset(this->_mqtt_publish_buffer,0, strlen(this->_mqtt_publish_buffer));
      memset(mqtt_temp_buff, 0, 15);
      split_string_char(this->_data_sensor_buffer, ',', 3, mqtt_temp_buff);
      sprintf(this->_mqtt_publish_buffer,"{ \"sensorRecords\": [{\"dataStreamId\": \"%d\",\"result\": \"%s %s\"}] }", id_N2O, mqtt_temp_buff,"ppm");
      this->publish_to_mqtt_server();
      
      memset(this->_mqtt_publish_buffer,0, strlen(this->_mqtt_publish_buffer));
      memset(mqtt_temp_buff, 0, 15);
      split_string_char(this->_data_sensor_buffer, ',', 4, mqtt_temp_buff);
      sprintf(this->_mqtt_publish_buffer,"{ \"sensorRecords\": [{\"dataStreamId\": \"%d\",\"result\": \"%s %s\"}] }", id_Pressure, mqtt_temp_buff,"Pa");
      this->publish_to_mqtt_server();
      
      memset(this->_mqtt_publish_buffer,0, strlen(this->_mqtt_publish_buffer));
      memset(mqtt_temp_buff, 0, 15);
      split_string_char(this->_data_sensor_buffer, ',', 5, mqtt_temp_buff);
      sprintf(this->_mqtt_publish_buffer,"{ \"sensorRecords\": [{\"dataStreamId\": \"%d\",\"result\": \"%s %s\"}] }", id_Temp, mqtt_temp_buff,"oC");
      this->publish_to_mqtt_server();

      memset(this->_mqtt_publish_buffer,0, strlen(this->_mqtt_publish_buffer));
      memset(mqtt_temp_buff, 0, 15);
      split_string_char(this->_data_sensor_buffer, ',', 6, mqtt_temp_buff);
      sprintf(this->_mqtt_publish_buffer,"{ \"sensorRecords\": [{\"dataStreamId\": \"%d\",\"result\": \"%s %s\"}] }", id_Hum, mqtt_temp_buff,"%");
      this->publish_to_mqtt_server();
      // publishData();
    }
  }
  
  if (!client.connected()) 
  {
    this->mqtt_reconnect();
  }
  client.loop();
}
/**
 * @brief ...
 *
 */
void Node::handle_data_from_actuator_node()
{
  if (Serial2.available())
  {
    vTaskDelay(20);
    char c;
    int i = 0;
    memset(this->_actuator_buffer, 0, SETTING_MAX_BUFFER);
    // String command = "";
    while (Serial2.available())
    {
      c = Serial2.read();
      // if (c == '\n') break;
      this->_actuator_buffer[i] = c;
      i++;
    }
    Serial.printf("Data from actuator:%s\n",this->_actuator_buffer);

    if (this->_device_role == NODE_SINK)
    {
      int msg = ACTUATOR_DATA_SIGNAL;
      if (xQueueSend(this->_cmd_msg_queue, (void *)&msg, 100) != pdTRUE)
      {
        Serial.println("From actuator node: CMD event queue is full");
      }
    }
    else if (this->_device_role == NODE_USER)
    {
      memset(helloPacket->data_str, 0, 50);
      sprintf(helloPacket->data_str,"SEND_DTS,%s", this->_actuator_buffer);
      
      if (this->_device_flags.sink_addr_available_flag == ENABLE)
      {
        Radio.createPacketAndSend(this->_sink_node_addr, helloPacket, 1);
        vTaskDelay(2000);
      }
      else
      {
        Serial.println("Error: sink node was not found!");
      }
    }
  }
}
/**
 * @brief ...
 *
 */
int Node::handle_cmd()
{
  // Serial.println("Pass here?");
  int item;
  if (xQueueReceive(this->_cmd_msg_queue, (void *)&item, portMAX_DELAY) == pdTRUE)
  {
    if (item == SERIAL_DATA_SIGNAL)
    {
      Serial.println("From serial");
      this->_device_flags.current_data_signal = SERIAL_DATA_SIGNAL;
      strcpy(this->_cmd_buffer, this->_serial_buffer);
    }
    else if (item == BUTTONS_DATA_SIGNAL)
    {
      Serial.println("From buttons");
      this->_device_flags.current_data_signal = BUTTONS_DATA_SIGNAL;
      strcpy(this->_cmd_buffer, this->_buttons_buffer);
    }
    else if (item == DISPLAY_DATA_SIGNAL)
    {
      Serial.println("From display");
      this->_device_flags.current_data_signal = DISPLAY_DATA_SIGNAL;
      strcpy(this->_cmd_buffer, this->_display_buffer);
    }
    else if (item == LORA_NODE_DATA_SIGNAL)
    {
      Serial.println("From LoRa");
      this->_device_flags.current_data_signal = LORA_NODE_DATA_SIGNAL;
      strcpy(this->_cmd_buffer, this->_LoRa_buffer);
    }
    else if (item == ACTUATOR_DATA_SIGNAL)
    {
      Serial.println("From actuator node");
      this->_device_flags.current_data_signal = ACTUATOR_DATA_SIGNAL;
      strcpy(this->_cmd_buffer, this->_actuator_buffer);
    }
    this->process_cmd();
  }
  return 0;
}

/**
 * @brief ...
 *
 */
void Node::display_print_nodes(int type)
{
  memset(this->_temp_cmd_buffer, 0, 50);

  if (type == DSP_DOWN)
  {
    if (this->_dsp_current_node_addr + 6 < LORA_MAX_NODE_ADDRESS) this->_dsp_current_node_addr += 6;
  }
  if (type == DSP_UP)
  {
    if (this->_dsp_current_node_addr - 6 >= 0) this->_dsp_current_node_addr -= 6;
  }
  for (int i = 1; i < 7; i++)
  {
    if (this->_dsp_node_addr[this->_dsp_current_node_addr+i-1] > 0)
    {
      sprintf(this->_temp_cmd_buffer, "b%d.txt=\"%x\"", i, this->_dsp_node_addr[this->_dsp_current_node_addr+i-1]);
      this->_display.send_data_to_display(this->_temp_cmd_buffer);
      sprintf(this->_temp_cmd_buffer, "b%d.bco=%d", i, 27967); // 27967 is the color code of the param in display
      this->_display.send_data_to_display(this->_temp_cmd_buffer);
    }
  }
}

void Node::get_chosen_node()
{
  int node_num = 0;
  char node[6] = {0,0,0,0,0,0};
  split_string_char(this->_cmd_buffer, ',', 1, node);
  node_num = convert_string_to_int(node, strlen(node));
  
  if (this->_dsp_node_addr[this->_dsp_current_node_addr+node_num-1] > 0)
  {
    this->_display.send_data_to_display("page param_st_page");
  }
}

/**
 * @brief ...
 *
 */
int Node::process_cmd()
{
  if (str_startswith(this->_cmd_buffer,"PRINT_ROUTING_TABLE"))
  {
    RoutingTableService::printRoutingTable();
  }
  else if (str_startswith(this->_cmd_buffer,"SEND_BROADCAST"))       // need revise, cmd to test sending dummy data to a specific node, SEND_TO,(node address)
  {
    strcpy(helloPacket->data_str, "Hello broadcast");
    // //Create packet and send it.
    Radio.createPacketAndSend(BROADCAST_ADDR, helloPacket, 1);
  }
  else if (str_startswith(this->_cmd_buffer,"SEND_TO"))       // for sink nodes, SEND_TO,(node address), cmd
  {
    // strcpy(helloPacket->data_str, "Hello from 6A08");
    char addr[10];
    memset(addr, 0, 10);
    split_string_char(this->_cmd_buffer, ',', 1, addr);
    uint16_t hex_int = strtol(addr, NULL, 16);

    int char_pos = get_index_from_string(this->_cmd_buffer, ',', 1);
    strcpy(helloPacket->data_str, this->_cmd_buffer + (char_pos+1));
    Serial.println(helloPacket->data_str);
    // //Create packet and send it.
    Radio.createPacketAndSend(hex_int, helloPacket, 1);
  }
  else if (str_startswith(this->_cmd_buffer,"SEND_CMD")) // for user node
  {
    char data_str[30];
    memset(data_str, 0, 30);
    strcpy(data_str, _cmd_buffer);

    int pos = strlen("SEND_CMD");
    char temp_str[30];
    memset(temp_str, 0, 30);
    strncpy(temp_str,data_str+(pos+1),strlen(data_str));
    Serial.println(temp_str);
    Serial2.println(temp_str);
  }
  else if (str_startswith(this->_cmd_buffer,"SDS")) // SEND Data To Sink : for Sink node
  {
    int pos = get_index_from_string(this->_cmd_buffer, ',', 0);
    this->_database.append_file(SD, SENSOR_RECORD_FILE_PATH, this->_cmd_buffer + pos + 1,true);
    Serial.println("Data sensor saved to file");
    strcpy(this->_data_sensor_buffer,this->_cmd_buffer);
    int msg = SEND_SENSOR_DATA_SIGNAL;

    if (xQueueSend(this->_mqtt_msg_queue, (void *)&msg, 100) != pdTRUE)
    {
      Serial.println("From cmd: mqtt event queue is full");
    }
  }
  else if (str_startswith(this->_cmd_buffer, "SET_RTC_TIME")) // SET_RTC_TIME,hours,minutes,seconds,day,month,year 
  {
    this->handle_set_date_time();
  }
  else if (str_startswith(this->_cmd_buffer, "GET_SINK_NODE")) // GET_SINK_NODE
  {
    this->handle_get_sink_node_addr();
  }
  else if (str_startswith(this->_cmd_buffer, "SET_SINK_NODE")) // SET_SINK_NODE,(sink node address)
  {
    this->handle_set_sink_node_addr();
  }
  else if (str_startswith(this->_cmd_buffer, "SHOW_DATE_TIME")) // SHOW_DATE 
  {
    this->_rtc.print_get_data_time();
  }
  else if (str_startswith(this->_cmd_buffer, "GET_NODE_ROLE")) // GET_NODE_ROLE
  {
    this->handle_get_node_role();
  }
  else if (str_startswith(this->_cmd_buffer, "CHANGE_NODE_ROLE")) // CHANGE_NODE_ROLE,(number) : number from 0 to 4
  { 
    this->handle_change_node_role();
  }
  else if (str_startswith(this->_cmd_buffer, "LIST_DIR")) // LIST_DIR
  { 
    this->_database.list_directory(SD, "/", 3);
  }
  else if (str_startswith(this->_cmd_buffer, "GET_NODE_ROLE")) // GET_NODE_ROLE
  { 
    this->handle_get_node_role();
  }
  else if (str_startswith(this->_cmd_buffer, "SN_ETR_PN")) // GET_NODE_ROLE
  {
    get_routing_node_address();
    this->display_print_nodes(DSP_ENTER);
  }
  else if (str_startswith(this->_cmd_buffer, "SN_FNL")) // GET_NODE_ROLE
  {
    this->display_print_nodes(DSP_UP);
  }
  else if (str_startswith(this->_cmd_buffer, "SN_FNR")) // GET_NODE_ROLE
  {
    this->display_print_nodes(DSP_DOWN);
  }
  else if (str_startswith(this->_cmd_buffer, "SN_NODE")) // GET_NODE_ROLE
  {
    this->get_chosen_node();
  }
  else if (str_startswith(this->_cmd_buffer, "DSP_GET_DT")) // for Display
  {
    this->display_print_date_time();
  }
  else if (str_startswith(this->_cmd_buffer, "DSP_BP_E")) // for Display
  {
    this->handle_print_data_bucket(DSP_ENTER);
  }
  else if (str_startswith(this->_cmd_buffer, "DSP_BP_H")) // for Display
  {
    this->handle_print_data_bucket(DSP_UP);
  }
  else if (str_startswith(this->_cmd_buffer, "DSP_BP_D")) // for Display
  {
    this->handle_print_data_bucket(DSP_DOWN);
  }
  else if (str_startswith(this->_cmd_buffer, "WIFI_ST")) // WIFI_ST,(ssid),(password) : set wifi ssid and password for sink node
  {
    this->handle_get_wifi_params();
  }
  else if (str_startswith(this->_cmd_buffer, "MQTT_ST")) // MQTT_ST,(broker),(port),(usr_name),(pass),(topic) : set mqtt params for sink node
  {
    this->handle_get_mqtt_params();
  }
  else if (str_startswith(this->_cmd_buffer, "MQTT_RESETUP")) // MQTT_RESETUP
  {
    this->mqtt_setup();
  }
  // else if (str_startswith(this->_buffer, "DSP_PAGE_BUCKET")) // for Display
  // {
  //   this->handle_print_data_bucket();
  // }
  else
  {
    Serial.println("Invalid command! Check is correct syntax");
  }
  return 0;
}
/**
 * @brief ...
 *
 */
int Node::create_task(TaskFunction_t pv_task_code, 
                          const char * name, 
                          const unsigned int stack_size, 
                          void *const param, 
                          UBaseType_t priority, 
                          TaskHandle_t *const task_handler, 
                          const int core)
{
  BaseType_t task_creation_result = xTaskCreatePinnedToCore(
    pv_task_code, 
    name, 
    stack_size, 
    param, 
    priority, 
    task_handler,
    core);

  if (task_creation_result == pdPASS) 
  {
    return DEVICE_SUCCESS_INIT_DISPLAY_TASK;
  }

  return DEVICE_ERR_INIT_DISPLAY_TASK;  
}


/**
 * @brief Setup for LoRa mesher
 *
 * @param data
 */

void Node::setup_Lora_mesher()
{
  //Get the configuration of the LoRaMesher
  pinMode(2, OUTPUT);
  LoraMesher::LoraMesherConfig config = LoraMesher::LoraMesherConfig();

  //Set the configuration of the LoRaMesher (TTGO T-BEAM v1.1)
  config.loraCs = 2;
  config.loraRst = 4;
  config.loraIrq = 15;
  config.loraIo1 = 5;

  config.loraCs = 5;
  config.loraRst = 4;
  config.loraIrq = 2;
  config.loraIo1 = 15;
  
  // config.loraCs = 13;
  // config.loraRst = 4;
  // config.loraIrq = 15;
  // config.loraIo1 = 5;

  newSPI.begin();
  config.spi = &newSPI; //sck, miso, mosi

  config.freq = 434.0;

  config.module = LoraMesher::LoraModules::SX1278_MOD;

  //Init the loramesher with a configuration
  Radio.begin(config);

  //Create the receive task and add it to the LoRaMesher
  // createReceiveMessages();
  this->create_task(
      this->user_recieve_packet_task,
      "Receive App Task",
      4096,
      (void*) 1,
      2,
      &receiveLoRaMessage_Handle,0);

  //Set the task handle to the LoRaMesher
  Radio.setReceiveAppDataTaskHandle(receiveLoRaMessage_Handle);

  //Start LoRaMesher
  Radio.start();

  Serial.println("Lora initialized");
}

/**
 * @brief Print the counter of the packet
 *
 * @param data
 */
void Node::print_packet(dataPacket data) 
{
  Serial.printf("Packet content received:%s\n", data.data_str);
}

/**
 * @brief Iterate through the payload of the packet and print the counter of the packet
 *
 * @param packet
 */
void Node::print_data_packet(AppPacket<dataPacket>* packet) 
{
  Serial.printf("Packet arrived from %X with size %d\n", packet->src, packet->payloadSize);

  //Get the payload to iterate through it
  dataPacket* dPacket = packet->payload;
  size_t payloadLength = packet->getPayloadLength();

  for (size_t i = 0; i < payloadLength; i++) 
  {
    //Print the packet
    print_packet(dPacket[i]);
  }
}

template<typename T>
static void reset_array(T arr, int len)
{
  for (int i = 0; i < len;i++)
    arr[i] = 0;
}

void Node::get_routing_node_address() 
{
  //Set the routing table list that is being used and cannot be accessed (Remember to release use after usage)
  reset_array(this->_dsp_node_addr,LORA_MAX_NODE_ADDRESS);
  LM_LinkedList<RouteNode>* routingTableList = Radio.routingTableListCopy();

  routingTableList->setInUse();

  // Screen.changeSizeRouting(Radio.routingTableSize());

  char text[15];
  for (int i = 0; i < Radio.routingTableSize(); i++)
  {
    RouteNode* rNode = (*routingTableList)[i];
    NetworkNode node = rNode->networkNode;
    snprintf(text, 15, ("|%X(%d)->%X"), node.address, node.metric, rNode->via);
    this->_dsp_node_addr[i] = node.address;
    // Screen.changeRoutingText(text, i);
  }

  //Release routing table list usage.
  routingTableList->releaseInUse();

  //Delete the routing table list
  delete routingTableList;

  // Screen.changeLineFour();
}

char temp_received_msg_buff[100];

void Node::handle_process_received_message()
{
  /* Wait for the notification of processReceivedPackets and enter blocking */
  ulTaskNotifyTake(pdPASS, portMAX_DELAY);

  //Iterate through all the packets inside the Received User Packets Queue
  while (Radio.getReceivedQueueSize() > 0) 
  {
    Serial.println("ReceivedUserData_TaskHandle notify received");
    Serial.printf("Queue receiveUserData size: %d\n", Radio.getReceivedQueueSize());

    //Get the first element inside the Received User Packets Queue
    AppPacket<dataPacket>* packet = Radio.getNextAppPacket<dataPacket>();
    // led_Flash(3, 200);
    //Print the data packet
    print_data_packet(packet);

    if (packet->dst == BROADCAST_ADDR)
    {
      // do something
    }
    else
    {

    }

    dataPacket* dPacket = packet->payload;
    strcpy(this->_LoRa_buffer, dPacket[0].data_str);
    Serial.println(this->_LoRa_buffer);
    if (str_startswith(this->_LoRa_buffer, "MSG00"))
    {
      if (this->_device_role == NODE_USER)
        Serial2.println("MSG00");
    }
    else if (str_startswith(this->_LoRa_buffer,"SDTS")) // SEND Data To Sink : for Sink node
    {
      int pos = get_index_from_string(this->_LoRa_buffer, ',', 0);
      Serial.println(this->_LoRa_buffer + (pos+1));
      memset(temp_received_msg_buff,0,100);
      strcpy(temp_received_msg_buff, this->_LoRa_buffer + (pos+1));
      memset(this->_LoRa_buffer,0,100);
      strcpy(this->_LoRa_buffer, temp_received_msg_buff);
      
      int msg = LORA_NODE_DATA_SIGNAL;
      if (xQueueSend(this->_cmd_msg_queue, (void *)&msg, 100) != pdTRUE)
      {
        Serial.println("From LoRa: CMD event queue is full");
      }
      memset(helloPacket->data_str, 0, 75);
      strcpy(helloPacket->data_str, "MSG00");
      Radio.createPacketAndSend(packet->src, helloPacket, 1);
    }
    if (str_startswith(this->_LoRa_buffer,"SDTU")) // SEND Data To Node : for User node
    {
      int pos = get_index_from_string(this->_LoRa_buffer, ',', 0);
      Serial.println(this->_LoRa_buffer + (pos+1));

      if (str_startswith(this->_LoRa_buffer + (pos+1), "UN_")) // header has this format means the data is for user node, not for actuator node
      {

      }
      else  // send to actuator node
      {
        Serial2.println(this->_LoRa_buffer + (pos+1));
      }

      memset(helloPacket->data_str, 0, 50);
      strcpy(helloPacket->data_str, "MSG00");
      Radio.createPacketAndSend(packet->src, helloPacket, 1);
    }

    //Create packet and send it.
    // Radio.createPacketAndSend(BROADCAST_ADDR, helloPacket, 1);
    //Delete the packet when used. It is very important to call this function to release the memory of the packet.
    Radio.deletePacket(packet);
  }
}

/**
 * @brief Create a Receive Messages Task and add it to the LoRaMesher
 *
 */
// void createReceiveMessages()
// {
//   int res = xTaskCreate(
//       processReceivedPackets,
//       "Receive App Task",
//       4096,
//       (void*) 1,
//       2,
//       &receiveLoRaMessage_Handle);

//     if (res != pdPASS)
//     {
//         Serial.printf("Error: Receive App Task creation gave error: %d\n", res);
//     }
// }

