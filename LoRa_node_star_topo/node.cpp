#include "WiFi.h"
#include <PubSubClient.h>
#include "SPI.h"
#include "node.h"
#include "spiffs_utilities.h"
#include "LoRa_star_topo_service.h"

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
// TaskHandle_t mqtt_task_handler         = NULL;

// char Dsp_node_addr_label[][10] = {"b1.txt=\"","b2.txt=\"","b3.txt=\"","b4.txt=\"","b5.txt=\"","b6.txt=\""};
// char Mqtt_json_str_format[] = {"{ \"sensorRecords\": [{\"dataStreamId\": \"%d\",\"result\": \"%s %s\"}] }"};
// #define EVT_STR_MAX_LENGTH 40
// char Event_time_string_temp[EVT_STR_MAX_LENGTH];

// SPIClass newSPI(VSPI);
// LoraMesher& Radio = LoraMesher::getInstance();

char Dsp_MT_current_label[][10] = {"n6.val=","n7.val=","n8.val=","n9.val=","n10.val=","n11.val="};

WiFiClient espClient;
PubSubClient client(espClient);
LoraStar Node1;
/**
 * @brief ...
 *
 */
Node::Node()
{
  this->_device_role                              = NODE_SINK;
  this->_device_flags.current_data_signal         = NO_DATA_SIGNAL;
  this->_device_flags.sink_addr_available_flag    = DISABLE;
  this->_device_flags.mqtt_get_params_flag         = FAILED;
  this->_device_flags.wifi_get_params_flag         = FAILED;
  _device_flags.wifi_state_flag                   = FAILED;
  _device_flags.mqtt_state_flag                   = FAILED;

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
    Serial.println("--- Access SPIFFS successfully ---");
    if (readFile(SPIFFS, NODE_ROLE_FILE_PATH, node_role_str) == SUCCESS)
    {
      this->_device_role = convert_string_to_int(node_role_str, 1);
    }
    else
    {
      this->_device_role = NODE_USER;
    }
    Serial.printf("   Node role is : %d", this->_device_role);

    if (this->_device_role == NODE_USER)
    {
      if (readFile(SPIFFS, SINK_NODE_ADDRESS_FILE_PATH, sink_node_str) == SUCCESS)
      {
        this->_sink_node_addr = strtol(sink_node_str, NULL, 16);
        this->_device_flags.sink_addr_available_flag = ENABLE;
      }
    }
    Serial.println("--- *********************** ---");
  }
  // this->_device_role = NODE_SINK;
  
  this->helloPacket = new dataPacket;
  this->setup_Lora_star_topo();

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
      Serial.println("--- Use default wifi params ---");
      memset(this->_wifi_ssid, 0, WIFI_NAME_LEN);
      strcpy(this->_wifi_ssid, Default_ssid);
      memset(this->_wifi_password, 0, WIFI_PASS_LEN);
      strcpy(this->_wifi_password, Default_password);
      Serial.println("--- *********************** ---");
    }
    else
    {
      Serial.println("--- Use wifi params saved in the last time ---");
      this->_database.read_file(SD, WIFI_FILE_PATH, _cmd_buffer,&line);
      memset(this->_wifi_ssid, 0, WIFI_NAME_LEN);
      memset(this->_wifi_password, 0, WIFI_PASS_LEN);
      split_string_char(this->_cmd_buffer, ',',1,this->_wifi_ssid);
      split_string_char(this->_cmd_buffer, ',',2,this->_wifi_password);
      Serial.println("--- ************************************** ---");
    }
    if (this->_device_flags.mqtt_get_params_flag == FAILED)
    {
      Serial.println("--- Use default mqtt params ---");
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
      Serial.println("--- ********************** ---");
    }
    else
    {
      Serial.println("--- Use wifi params saved in the last time ---");
      this->_database.read_file(SD, MQTT_FILE_PATH, _cmd_buffer, &line);
      // memset(this->_mqtt_broker, 0, MQTT_BROKER_LEN);
      // memset(this->_mqtt_port, 0, MQTT_PORT_LEN);
      // memset(this->_mqtt_username, 0, MQTT_USERNAME_LEN);
      // memset(this->_mqtt_password, 0, MQTT_PASS_LEN);
      // memset(this->_mqtt_topic, 0, MQTT_TOPIC_LEN);
      split_string_char(this->_cmd_buffer, ',',1,this->_mqtt_broker);
      split_string_char(this->_cmd_buffer, ',',2,this->_mqtt_port);
      split_string_char(this->_cmd_buffer, ',',3,this->_mqtt_username);
      split_string_char(this->_cmd_buffer, ',',4,this->_mqtt_password);
      split_string_char(this->_cmd_buffer, ',',5,this->_mqtt_topic);
      Serial.println("--- ************************************** ---");
    }

    Serial.println("--- WiFi setup information: --- ");
    Serial.println(this->_wifi_ssid);
    Serial.println(this->_wifi_password);
    Serial.println("--- Mqtt setup information: --- ");
    Serial.println(this->_mqtt_broker);
    Serial.println(this->_mqtt_port);
    Serial.println(this->_mqtt_username);
    Serial.println(this->_mqtt_password);
    Serial.println(this->_mqtt_topic);
    Serial.println("--- *********************** --- ");
    // this->create_task(mqtt_task, "mqtt task", 8192, NULL,   4, &mqtt_task_handler, 0);
  }
  else if (this->_device_role == NODE_USER)
  {
    this->_rtc.begin();
    this->create_task(actuator_data_task, "actuator task", 8192, NULL,   4, &actuator_task_handler, 0);
  }
  // else if (this->_device_role == NODE_GATEWAY || this->_device_role == NODE_USER)
  // {
  // pinMode(13, OUTPUT);
  // pinMode(12, OUTPUT);
  // digitalWrite(13,LOW);
  // digitalWrite(12,LOW);
  // }
  this->create_task(cmd_task, "cmd task", 8192, NULL,   4, &cmd_task_handler, 0);
  this->create_task(serial_task, "serial task", 8192, NULL,   4, &serial_task_handler, 0);

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
    
    this->_serial_buffer[i] = 0;

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
  int role_number = strtol(this->_cmd_buffer + (pos+1), NULL, 10);
  
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
    // memset(time_buffer, 0, 5);
    split_string_char(this->_cmd_buffer, ',', i+1, time_buffer);
    date_time_arr[i] = convert_string_to_int(time_buffer, strlen(time_buffer));
  }
  
  this->_rtc.set_time(date_time_arr[0], date_time_arr[1], date_time_arr[2], date_time_arr[3], date_time_arr[4], date_time_arr[5]);
  
  if (this->_device_flags.current_data_signal == DISPLAY_DATA_SIGNAL)
  {
    this->_display.send_data_to_display("st_label.txt=\"OK\"");
  }

  Serial.println("Set date time done");
}
/**
 * @brief ...
 *
 */
void Node::display_print_date_time() // this function becomes hybrid (add updating wifi state)
{
  // char temp[3];
  // memset(_temp_cmd_buffer, 0, strlen(_temp_cmd_buffer));
  sprintf(_temp_cmd_buffer, "t14.txt=\"%d:%d\"",this->_rtc.get_hours(), this->_rtc.get_minutes());
  this->_display.send_data_to_display(_temp_cmd_buffer);
  // memset(_temp_cmd_buffer, 0, strlen(_temp_cmd_buffer));
  sprintf(_temp_cmd_buffer, "date_lb.txt=\"%d/%d/%d\"",this->_rtc.get_day(), this->_rtc.get_month(), this->_rtc.get_year());
  this->_display.send_data_to_display(_temp_cmd_buffer);

  if (_device_flags.wifi_state_flag == SUCCESS)
  {
    this->_display.send_data_to_display("p1.pic=16");
  }
  else
  {
    this->_display.send_data_to_display("p1.pic=17");
  }
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
    if (_dsp_current_line_bucket - 5 >= 0) _dsp_current_line_bucket -= 5;
  }
  else if (type == DSP_DOWN)
  {
    if (_dsp_current_line_bucket + 5 <= this->_sensor_file_num_of_lines) _dsp_current_line_bucket += 5;
  }
  Serial.printf("------- Current line %d, total line %d\n",_dsp_current_line_bucket,_sensor_file_num_of_lines);

  // memset(this->_cmd_buffer, 0, SERIAL_INPUT_MAX_BUFFER);
  // memset(this->_dsp_bucket_buffer, 0, DISPLAY_BUCKET_MAX_BUFFER);

  this->_database.read_line(SD, SENSOR_RECORD_FILE_PATH, this->_dsp_bucket_buffer, _dsp_current_line_bucket);
  sprintf(this->_cmd_buffer,"t1.txt=\"%s\\r\"", this->_dsp_bucket_buffer);
  this->_display.send_data_to_display(this->_cmd_buffer);
  
  for (int i = 1; i < 5; i++)
  {
    // memset(this->_cmd_buffer, 0, SERIAL_INPUT_MAX_BUFFER);
    // memset(this->_dsp_bucket_buffer, 0, DISPLAY_BUCKET_MAX_BUFFER);
    // strcat(this->_cmd_buffer,"t1.txt+=\"");
    if (_dsp_current_line_bucket+i <= this->_sensor_file_num_of_lines)
    {
      this->_database.read_line(SD, SENSOR_RECORD_FILE_PATH, this->_dsp_bucket_buffer, _dsp_current_line_bucket+i);
    }
    if (strlen(this->_dsp_bucket_buffer) != 0 )
    {
      sprintf(this->_cmd_buffer,"t1.txt+=\"%s\\r\"", this->_dsp_bucket_buffer);
      this->_display.send_data_to_display(this->_cmd_buffer);
    }
  }
}
/**
 * @brief ...
 *
 */
void Node::handle_get_wifi_params()
{
  // memset(this->_wifi_ssid,0,WIFI_NAME_LEN);
  split_string_char(this->_cmd_buffer,',',1,this->_wifi_ssid);
  // memset(this->_wifi_password,0,WIFI_NAME_LEN);
  split_string_char(this->_cmd_buffer,',',2,this->_wifi_password);

  // notify to users
  if (this->_database.write_file(SD, WIFI_FILE_PATH, this->_cmd_buffer, false) == DATABASE_SUCCESS_WRITE_FILE)
  {
    if (this->_device_flags.current_data_signal == DISPLAY_DATA_SIGNAL)
    {
      handle_get_wifi_ssid();
      this->_display.send_data_to_display(this->_temp_cmd_buffer);
    }
    else if (this->_device_flags.current_data_signal == SERIAL_DATA_SIGNAL)
    {
      Serial.println("Set wifi params done");
    }
  }
  else
  {
    if (this->_device_flags.current_data_signal == DISPLAY_DATA_SIGNAL)
    {
      // handle_get_wifi_ssid();
      this->_display.send_data_to_display("wifi_err_lb.txt=\"Save param failed!\"");
    }
    else if (this->_device_flags.current_data_signal == SERIAL_DATA_SIGNAL)
    {
      Serial.println("Save wifi params failed!");
    }
  }
}

void Node::handle_get_wifi_ssid()
{
  // memset(_temp_cmd_buffer,0, MAX_BUFFER_LENGTH);
  sprintf(_temp_cmd_buffer,"wifi_st_lb.txt=\"Connected to: %s\"",this->_wifi_ssid);
  this->_display.send_data_to_display(_temp_cmd_buffer);
}
/**
 * @brief ...
 *
 */
int Node::connect_to_wifi(const char *ssid, const char *pass)
{
  Serial.println("Connecting to WiFi..");

  WiFi.begin(ssid, pass);
  _device_flags.wifi_state_flag = FAILED;
  while (WiFi.status() != WL_CONNECTED) 
  {
    vTaskDelay(500);
    Serial.print(".");
  }
  _device_flags.wifi_state_flag = SUCCESS;
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

void Node::handle_get_mqtt_params() // note: this function is desired to modify for optimal
{
  // memset(this->_temp_cmd_buffer, 0, MAX_BUFFER_LENGTH);
  split_string_char(_cmd_buffer,',',1,_temp_cmd_buffer);
  check_and_store_string(_temp_cmd_buffer, _mqtt_broker);

  // memset(this->_temp_cmd_buffer, 0, MAX_BUFFER_LENGTH);
  split_string_char(_cmd_buffer,',',2,_temp_cmd_buffer);
  check_and_store_string(_temp_cmd_buffer, _mqtt_port);

  // memset(this->_temp_cmd_buffer, 0, MAX_BUFFER_LENGTH);
  split_string_char(_cmd_buffer,',',3,_temp_cmd_buffer);
  check_and_store_string(_temp_cmd_buffer, _mqtt_username);

  // memset(this->_temp_cmd_buffer, 0, MAX_BUFFER_LENGTH);
  split_string_char(_cmd_buffer,',',4,_temp_cmd_buffer);
  check_and_store_string(_temp_cmd_buffer, _mqtt_password);

  // memset(this->_temp_cmd_buffer, 0, MAX_BUFFER_LENGTH);
  split_string_char(_cmd_buffer,',',5,_temp_cmd_buffer);
  check_and_store_string(_temp_cmd_buffer, _mqtt_topic);

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
  this->_display.send_data_to_display("st_label.txt=\"OK\"");
}

void Node::send_mqtt_data_info() // note: this function is desired to modify for optimal
{
  // memset(this->_temp_cmd_buffer, 0, MAX_BUFFER_LENGTH);
  sprintf(this->_temp_cmd_buffer,"t1.txt=\"%s\"", this->_mqtt_broker);
  this->_display.send_data_to_display(this->_temp_cmd_buffer);
  
  // memset(this->_temp_cmd_buffer, 0, MAX_BUFFER_LENGTH);
  sprintf(this->_temp_cmd_buffer,"t7.txt=\"%s\"", this->_mqtt_port);
  this->_display.send_data_to_display(this->_temp_cmd_buffer);
  
  // memset(this->_temp_cmd_buffer,0,MAX_BUFFER_LENGTH);
  sprintf(this->_temp_cmd_buffer,"t8.txt=\"%s\"", this->_mqtt_username);
  this->_display.send_data_to_display(this->_temp_cmd_buffer);
  
  // memset(this->_temp_cmd_buffer,0,MAX_BUFFER_LENGTH);
  sprintf(this->_temp_cmd_buffer,"t9.txt=\"%s\"", this->_mqtt_password);
  this->_display.send_data_to_display(this->_temp_cmd_buffer);

  // memset(this->_temp_cmd_buffer,0,MAX_BUFFER_LENGTH);
  sprintf(this->_temp_cmd_buffer,"t10.txt=\"%s\"", this->_mqtt_topic);
  this->_display.send_data_to_display(this->_temp_cmd_buffer);
  
  if (_device_flags.mqtt_state_flag == SUCCESS)
  {
    this->_display.send_data_to_display("p0.pic=19");
    this->_display.send_data_to_display("t12.txt=\"connected\"");
  }
  else
  {
    this->_display.send_data_to_display("p0.pic=18");
    this->_display.send_data_to_display("t12.txt=\"disconnect\"");
  }

}

/**
 * @brief Get data from mqtt server that the device connect to
 * @note The function is not complete
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
  _device_flags.mqtt_state_flag = FAILED;
  while(!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("esp8266-client" , this->_mqtt_username, this->_mqtt_password)) 
    {
      _device_flags.mqtt_state_flag = SUCCESS;
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

// char mqtt_temp_buff[15];

void Node::handle_mqtt_communication() // note: this function needs to optimise
{
  int item;
  if (xQueueReceive(this->_mqtt_msg_queue, (void *)&item, 0) == pdTRUE)
  {
    // do something
    if (item == SEND_SENSOR_DATA_SIGNAL)
    {
      // memset(this->_mqtt_publish_buffer, 0, strlen(this->_mqtt_publish_buffer));
      // memset(_temp_cmd_buffer, 0, 15);
      split_string_char(this->_data_sensor_buffer, ',', 1, _temp_cmd_buffer);
      sprintf(this->_mqtt_publish_buffer,"{ \"sensorRecords\": [{\"dataStreamId\": \"%d\",\"result\": \"%s %s\"}] }", id_CO2, _temp_cmd_buffer,"ppm");
      this->publish_to_mqtt_server();

      // memset(this->_mqtt_publish_buffer, 0, strlen(this->_mqtt_publish_buffer));
      // memset(_temp_cmd_buffer, 0, 15);
      split_string_char(this->_data_sensor_buffer, ',', 2, _temp_cmd_buffer);
      sprintf(this->_mqtt_publish_buffer,"{ \"sensorRecords\": [{\"dataStreamId\": \"%d\",\"result\": \"%s %s\"}] }", id_CH4, _temp_cmd_buffer,"ppm");
      this->publish_to_mqtt_server();
      
      // memset(this->_mqtt_publish_buffer, 0, strlen(this->_mqtt_publish_buffer));
      // memset(_temp_cmd_buffer, 0, 15);
      split_string_char(this->_data_sensor_buffer, ',', 3, _temp_cmd_buffer);
      sprintf(this->_mqtt_publish_buffer,"{ \"sensorRecords\": [{\"dataStreamId\": \"%d\",\"result\": \"%s %s\"}] }", id_N2O, _temp_cmd_buffer,"ppm");
      this->publish_to_mqtt_server();
      
      // memset(this->_mqtt_publish_buffer, 0, strlen(this->_mqtt_publish_buffer));
      // memset(_temp_cmd_buffer, 0, 15);
      split_string_char(this->_data_sensor_buffer, ',', 4, _temp_cmd_buffer);
      sprintf(this->_mqtt_publish_buffer,"{ \"sensorRecords\": [{\"dataStreamId\": \"%d\",\"result\": \"%s %s\"}] }", id_Pressure, _temp_cmd_buffer,"Pa");
      this->publish_to_mqtt_server();
      
      // memset(this->_mqtt_publish_buffer, 0, strlen(this->_mqtt_publish_buffer));
      // memset(_temp_cmd_buffer, 0, 15);
      split_string_char(this->_data_sensor_buffer, ',', 5, _temp_cmd_buffer);
      sprintf(this->_mqtt_publish_buffer,"{ \"sensorRecords\": [{\"dataStreamId\": \"%d\",\"result\": \"%s %s\"}] }", id_Temp, _temp_cmd_buffer,"oC");
      this->publish_to_mqtt_server();

      // memset(this->_mqtt_publish_buffer, 0, strlen(this->_mqtt_publish_buffer));
      // memset(_temp_cmd_buffer, 0, 15);
      split_string_char(this->_data_sensor_buffer, ',', 6, _temp_cmd_buffer);
      sprintf(this->_mqtt_publish_buffer,"{ \"sensorRecords\": [{\"dataStreamId\": \"%d\",\"result\": \"%s %s\"}] }", id_Hum, _temp_cmd_buffer,"%");
      this->publish_to_mqtt_server();
      // publishData();
    }
  }
  
  if (!client.connected()) 
  {
    this->mqtt_reconnect();
  }
  if (WiFi.status() != WL_CONNECTED) 
  {
    connect_to_wifi(this->_wifi_ssid, this->_wifi_password); 
  }
  client.loop();
}
/**
 * @brief read data from serial port of actuator node
 *
 */
void Node::handle_data_from_actuator_node()
{
  if (Serial2.available())
  {
    vTaskDelay(20);
    char c;
    int i = 0;
    memset(this->_actuator_buffer, 0, MAX_BUFFER_LENGTH);
    // String command = "";
    while (Serial2.available())
    {
      c = Serial2.read();
      // if (c == '\n') break;
      this->_actuator_buffer[i] = c;
      i++;
    }
      
    this->_actuator_buffer[i] = 0;
    
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
      // memset(helloPacket->data_str, 0, 50);
      // sprintf(helloPacket->data_str,"SDTS,%s", this->_actuator_buffer);
      
      if (str_startswith(this->_actuator_buffer, "MFSN"))
      {
        if (this->_device_flags.sink_addr_available_flag == ENABLE)
        {
          Node1.create_packet_data_and_send(String(_sink_node_addr, HEX).c_str(), this->_actuator_buffer);
          // Radio.createPacketAndSend(this->_sink_node_addr, helloPacket, 1);
          vTaskDelay(250);
        }
        else
        {
          Serial.println("Error: sink node was not found!");
        }
      }
      else
      {
        int msg = ACTUATOR_DATA_SIGNAL;
        if (xQueueSend(this->_cmd_msg_queue, (void *)&msg, 100) != pdTRUE)
        {
          Serial.println("From actuator node: CMD event queue is full");
        }
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
    memset(_cmd_buffer,0, MAX_BUFFER_LENGTH);

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
      memset(_temp_cmd_buffer, 0, 10);
      split_string_char(this->_LoRa_buffer, '&', SRC_ADDR_POSITION, _temp_cmd_buffer); 
      _current_data_packet_info.source = strtol(_temp_cmd_buffer, NULL, 16);
      memset(_temp_cmd_buffer, 0, 10);
      split_string_char(this->_LoRa_buffer, '&', DES_ADDR_POSITION, _temp_cmd_buffer); 
      _current_data_packet_info.destination = strtol(_temp_cmd_buffer, NULL, 16);
      memset(_temp_cmd_buffer, 0, 10);
      split_string_char(this->_LoRa_buffer, '&', TYPE_POSITION, _temp_cmd_buffer); 
      _current_data_packet_info.type = strtol(_temp_cmd_buffer, NULL, 10);
      memset(_temp_cmd_buffer, 0, 10);
      split_string_char(this->_LoRa_buffer, '&', SIZE_POSITION, _temp_cmd_buffer); 
      _current_data_packet_info.size = strtol(_temp_cmd_buffer, NULL, 10);

      Serial.println("From LoRa:");
      this->_device_flags.current_data_signal = LORA_NODE_DATA_SIGNAL;
      // process the data packet that is on string format
      int payload_pos = get_index_from_string(_LoRa_buffer, ',', 0) + 1;
      strcpy(_cmd_buffer, _LoRa_buffer + payload_pos);
      Serial.println(this->_cmd_buffer);
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
  // memset(this->_temp_cmd_buffer, 0, MAX_BUFFER_LENGTH);

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
  // int node_num = 0;
  char node[6] = {0,0,0,0,0,0};
  split_string_char(this->_cmd_buffer, ',', 1, node);
  _current_node_str = String(node);
  // node_num = convert_string_to_int(node, strlen(node));
  if (!_current_node_str.startsWith("null"))
  {
    this->_display.send_data_to_display("page param_st_page");
    print_current_node_data_sensor();
  }

  // add code to print lastest data collected.
  void print_node_lasted_data_sensor();
}

void Node::store_current_node_data_sensor()
{
  // convert_int_to_str(_current_data_packet_info.source, _cmd_buffer);
  uint8_t temp;
  uint8_t attempt_num = 5;
  char node_file_dir[10];
  memset(node_file_dir, 0, 10);
  // convert_int_to_str(_current_data_packet_info.source, _temp_cmd_buffer);
  sprintf(_temp_cmd_buffer, "/%s%s", _current_node_str.c_str(), SENSOR_RECORD_FILE_PATH);
  File file = SD.open(_temp_cmd_buffer);

  if (!file)
  {
    Serial.printf("File %s didn't exist, attempt to create file...\n", _temp_cmd_buffer);
    sprintf(node_file_dir,"/%s",_current_node_str.c_str());
    _database.create_dir(SD, node_file_dir);
    while (attempt_num-- > 0)
    {

      if(_database.write_file(SD, _temp_cmd_buffer, _cmd_buffer, false) == DATABASE_SUCCESS_WRITE_FILE)
      {
        Serial.printf("Create file %s successfully!\n", _temp_cmd_buffer);
        break;
      }
      vTaskDelay(20);
    }
    if (attempt_num == 0)
    {
      Serial.println("Attempt storing data to node file failed! Ignore.");
      _database.append_file(SD, _temp_cmd_buffer, _cmd_buffer, false);
    }
    else
    {
      Serial.println("Append data to node file success.");
    }
  }
  else
  {
    Serial.printf("File %s has been already created, attempt data to file...\n");
    if (_database.append_file(SD, _temp_cmd_buffer, _cmd_buffer, false))
    {
      Serial.println("Append data to node file success.");
    }
    else
    {
      Serial.println("Append data to node file failed, ignore.");
    }
  }
}

void Node::print_current_node_data_sensor()
{
  sprintf(_temp_cmd_buffer, "node_addr.txt=\"%s\"",_current_node_str.c_str());
  this->_display.send_data_to_display(_temp_cmd_buffer);
}

void Node::print_node_lasted_data_sensor()
{
  // char node_file_path[20];
  // memset(node_file_path, 0, 20);
  int temp = 0;
  sprintf(_temp_cmd_buffer, "/%s%s", _current_node_str.c_str(), SENSOR_RECORD_FILE_PATH);
  File file = SD.open(_temp_cmd_buffer);
  _database.get_number_of_line(SD, _temp_cmd_buffer, &temp);

  if (!file)
  {
    Serial.printf("Open file %s node data sensor failed! Ignore.\n", _temp_cmd_buffer);
  }
  else
  {
    if (temp <= 0)
    {
      sprintf(this->_cmd_buffer,"node_data.txt+=\"%s\\r\"", "EMPTY");
    }
    else
    {
      for (int i = temp; i >= temp-5; i--)
      {
        this->_database.read_line(SD, _temp_cmd_buffer, this->_dsp_bucket_buffer, i);
        sprintf(this->_cmd_buffer,"node_data.txt+=\"%s\\r\"", this->_dsp_bucket_buffer);
        this->_display.send_data_to_display(this->_cmd_buffer);
      }
    }
  }
}

void Node::handle_device_state_to_node(int state)
{
  if (!_current_node_str.startsWith("null"))
  {
    Serial.print("Turn device at address:"); Serial.print(_current_node_str); Serial.printf(" with state: %d",state);

    if (state == DEVICE_STATE_ON)
      Node1.create_packet_data_and_send(_current_node_str.c_str(), "MFAN,DEVICE_ON");
    else if (state == DEVICE_STATE_OFF)
      Node1.create_packet_data_and_send(_current_node_str.c_str(), "MFAN,DEVICE_OFF");
    else if (state == DEVICE_STATE_SAMPLE)
      Node1.create_packet_data_and_send(_current_node_str.c_str(), "MFAN,FORCE_EVENT_ON");
  }
  else
  {
    Serial.println("Current node is not set! Ignore");
  }
}

void Node::reset_current_node_str()
{
  _current_node_str = "null";
}

#define RESP_DEVICE_ON      0
#define RESP_DEVICE_OFF     1
#define RESP_DEVICE_SAMPLE  2
#define RESP_MT_GS          3
#define RESP_ET_GS          4
#define RESP_DS_GS          5
#define RESP_MT_GF          6
#define RESP_ET_GF          7
#define RESP_DS_GF          8

void Node::handle_node_response(int code_num)
{
  Serial.printf("Code num: %d", code_num);
  switch (code_num)
  {
    case RESP_DEVICE_ON:
    {
      _display.send_data_to_display("t1.txt=\"ON\"");
      break;
    }
    case RESP_DEVICE_OFF:
    {
      _display.send_data_to_display("t1.txt=\"OFF\"");
      break;
    }
    case RESP_DEVICE_SAMPLE:
    {
      _display.send_data_to_display("t1.txt=\"ON SAMPLE\"");
      break;
    }
    case RESP_MT_GS:
    {
      _display.send_data_to_display("st_label.txt=\"OK\"");
      break;
    }
    case RESP_ET_GS:
    {
      _display.send_data_to_display("st_label.txt=\"OK\"");
      break;
    }
    case RESP_DS_GS:
    {
      Serial2.println("MSG00");
      break;
    }
    case RESP_MT_GF:
    {
      _display.send_data_to_display("st_label.txt=\"ERROR\"");
      break;
    }
    case RESP_ET_GF:
    {
      _display.send_data_to_display("st_label.txt=\"ERROR\"");
      break;
    }
    case RESP_DS_GF:
    {
      Serial2.println("MSG01");
      break;
    }
    default:
    {

    }
  }
}

void Node::store_time_setting_data(int type)
{
  int i = get_index_from_string(_cmd_buffer, ',', 0) + 1;
  if (type == EVENT_TIME_DATA)
  {
    strcpy(_evt_buffer, _cmd_buffer+i);
    _evt_total = count_character(_evt_buffer, ',') + 1;
    
    Serial.println(_evt_buffer);
  }
  else
  {
    strcpy(_mt_buffer, _cmd_buffer+i);
    if (strlen(_mt_buffer) > 0)
    {
      _mt_buffer[strlen(_mt_buffer)-1] = 0;
    }
    Serial.println(_mt_buffer);
  }
}


void Node::display_update_event_time(int pos)
{
  int n, k = 4;
  char temp[10];
  split_string_char(this->_cmd_buffer, ',', 1, temp);

  if (pos == -1)
    n = strtol(temp, NULL, 10);
  else
    n = pos;

  for (int i = n-1; i < n+3; i++)
  {
    split_string_char(_evt_buffer, ',', i, temp);
    if (i >= _evt_total)
      sprintf(_temp_cmd_buffer,"b%d.txt=\"%s\"", k, "0:0");
    else
      sprintf(_temp_cmd_buffer,"b%d.txt=\"%s\"", k, temp);
    
    this->_display.send_data_to_display(_temp_cmd_buffer);
    k += 1;
  }
}

void Node::display_update_measure_time()
{
  unsigned char n = 6;
  char temp[10];
  char num_str[5];

  if (strlen(_mt_buffer) == 0)
  {
    Serial.println("Measure time buffer empty");
    return;
  }


  for (int i = 0; i < 3; i++)
  {
    split_string_char(_mt_buffer, ',', i, temp);

    split_string_char(temp, '-', 0, num_str);
    sprintf(_temp_cmd_buffer,"n%d.val=%s", n, num_str);
    this->_display.send_data_to_display(_temp_cmd_buffer);
    n += 1;
    
    split_string_char(temp, '-', 1, num_str);
    sprintf(_temp_cmd_buffer,"n%d.val=%s", n, num_str);
    this->_display.send_data_to_display(_temp_cmd_buffer);
    n += 1;
  }
}

void Node::handle_get_measure_event_time()
{
  sprintf(_temp_cmd_buffer, "MFAN,%s", _cmd_buffer);
  Node1.create_packet_data_and_send(_current_node_str.c_str(), _temp_cmd_buffer);
}

/**
 * @brief ...
 *
 */
int Node::process_cmd()
{
  if (str_startswith(this->_cmd_buffer, "DEVICE_ON"))
  {
    this->handle_device_state_to_node(DEVICE_STATE_ON);
    // Serial.println("PASS");
    // this->set_device_state(0);
    // if (this->_device_flags.current_data_signal == DISPLAY_DATA_SIGNAL){}
    //   this->display_update_device_state();
  }
  else if (str_startswith(this->_cmd_buffer, "DEVICE_OFF"))
  {
    this->handle_device_state_to_node(DEVICE_STATE_OFF);
    // this->set_device_state(1);
    // if (this->_device_flags.current_data_signal == DISPLAY_DATA_SIGNAL){}
    //   this->display_update_device_state();
  }
  else if (str_startswith(this->_cmd_buffer, "ET_RESP"))
  {
    this->store_time_setting_data(EVENT_TIME_DATA);
    this->display_update_event_time(1);
  }
  else if (str_startswith(this->_cmd_buffer, "MT_RESP"))
  {
    this->store_time_setting_data(MEASURE_TIME_DATA);
    this->display_update_measure_time();
    // this->handle_device_state_to_node(DEVICE_STATE_SAMPLE);
  }
  else if (str_startswith(this->_cmd_buffer, "ET_REQ"))
  {
    strcpy(_cmd_buffer,"MFAN,ET_REQ");
    Node1.create_packet_data_and_send(_current_node_str.c_str(), _cmd_buffer);
  }
  else if (str_startswith(this->_cmd_buffer, "MT_REQ"))
  {
    strcpy(_cmd_buffer,"MFAN,MT_REQ");
    Node1.create_packet_data_and_send(_current_node_str.c_str(), _cmd_buffer);
  }
  else if (str_startswith(this->_cmd_buffer, "FORCE_EVENT_ON"))
  {
    this->handle_device_state_to_node(DEVICE_STATE_SAMPLE);
    // this->set_device_state(2);
  }
  else if (str_startswith(this->_cmd_buffer,"PRINT_ROUTING_TABLE"))
  {
    Node1.print_node_address();
    // RoutingTableService::printRoutingTable();
  }
  else if (str_startswith(this->_cmd_buffer, "EVENT_TIME_SET")) // EVENT_TIME_SET,hours,minutes
  {
    this->handle_get_measure_event_time();
  }
  else if (str_startswith(this->_cmd_buffer, "ET_UPDATE")) // ET_UPDATE,position,hours,minutes
  {
    this->handle_get_measure_event_time();
  }
  else if (str_startswith(this->_cmd_buffer, "ET_DELETE")) // EVENT_DELETE,position
  {
    this->handle_get_measure_event_time();
  }
  else if (str_startswith(this->_cmd_buffer, "ET_DEL_TAIL")) // EVENT_DEL_TAIL
  {
    this->handle_get_measure_event_time();
  }
    else if (str_startswith(this->_cmd_buffer, "MAJOR_TIME_SET")) // MAJOR_TIME_SET,no_of_interval,minutes
  {
    this->handle_get_measure_event_time();
  }
  else if (str_startswith(this->_cmd_buffer, "MINOR_TIME_SET")) // MINOR_TIME_SET,no_of_interval,minutes
  {
    this->handle_get_measure_event_time();
  }
  else if (str_startswith(this->_cmd_buffer, "SPIKE_TIME_SET")) // SPIKE_TIME_SET,no_of_interval,seconds
  {
    this->handle_get_measure_event_time();
  }
  else if (str_startswith(this->_cmd_buffer,"SDS")) // Send Data Sensor 
  {
    // save to sd card
    String des_node_addr;
    int pos = get_index_from_string(this->_cmd_buffer, ',', 0);
    this->_database.append_file(SD, SENSOR_RECORD_FILE_PATH, this->_cmd_buffer + pos + 1, false);
    Serial.println("Data sensor saved to file");
    store_current_node_data_sensor();
    strcpy(this->_data_sensor_buffer,this->_cmd_buffer);
    // send response to node

    des_node_addr = String(_current_data_packet_info.source, HEX);
    Node1.create_packet_data_and_send(des_node_addr.c_str(), "MFAN,MSG00");

    // dispatch data to mqtt task to send to server
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
    this->_rtc.print_date_time();
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
  else if (str_startswith(this->_cmd_buffer, "SN_NODE")) // 
  {
    this->get_chosen_node();
  }
  else if (str_startswith(this->_cmd_buffer, "DSP_GET_DT")) // for Display
  {
    this->display_print_date_time();
  }
  else if (str_startswith(this->_cmd_buffer, "DSP_MQTT_INFO")) // for Display
  {
    this->send_mqtt_data_info();
  }
  else if (str_startswith(this->_cmd_buffer, "DSP_ET_UPDATE")) // for Display
  {
    this->display_update_event_time(-1);
  }
  else if (str_startswith(this->_cmd_buffer, "DSP_MT_UPDATE")) // for Display
  {
    this->display_update_measure_time();
  }
  else if (str_startswith(this->_cmd_buffer, "DSP_DSN")) // for Display
  {
    print_current_node_data_sensor();
    print_node_lasted_data_sensor();
    // store_current_node_data_sensor();
    // this->handle_print_data_sensor_node(uint16_t node_address);
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
  else if (str_startswith(this->_cmd_buffer, "GET_WIFI_SSID")) // WIFI_ST,(ssid),(password) : set wifi ssid and password for sink node
  {
    this->handle_get_wifi_ssid();
  }
  else if (str_startswith(this->_cmd_buffer, "MQTT_ST")) // MQTT_ST,(broker),(port),(usr_name),(pass),(topic) : set mqtt params for sink node
  {
    this->handle_get_mqtt_params();
  }
  else if (str_startswith(this->_cmd_buffer, "MQTT_RESETUP")) // MQTT_RESETUP
  {
    this->mqtt_setup();
  }
  else if (str_startswith(this->_cmd_buffer, "CLEAR_CURRNODE")) // 
  {
    this->reset_current_node_str();
  }
  else if (str_startswith(this->_cmd_buffer, "NRESP")) // Node RESPonds, 
  {
    // char code_num_str[10];
    // memset(_temp_cmd_buffer, 0, 10);
    split_string_char(_cmd_buffer,',', 1, _temp_cmd_buffer);
    Serial.println();
    Serial.println(_temp_cmd_buffer);
    Serial.println();
    int code_num = strtol(_temp_cmd_buffer, NULL, 10);
    this->handle_node_response(code_num);
    // this->handle_node_response(int code_num);
  }
  else if (str_startswith(this->_cmd_buffer, "RTC_DT_REQ"))
  {
    _rtc.get_date_time_string(_temp_cmd_buffer);
    sprintf(_cmd_buffer,"SET_RTC_TIME,%s", _temp_cmd_buffer);
    Serial.println(_cmd_buffer);
    Serial2.println(_cmd_buffer);
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

void Node::setup_Lora_star_topo()
{
  Node1._user_receive_packet_func = user_recieve_packet_task;
  Node1._role = _device_role;
  Node1.begin();
  // this->create_task(
  //     this->user_recieve_packet_task,
  //     "Receive App Task",
  //     4096,
  //     (void*) 1,
  //     2,
  //     &receiveLoRaMessage_Handle,0);

  Serial.println("Lora initialized");
}

/**
 * @brief Print the counter of the packet
 *
 * @param data
 */
void print_packet(char *data) 
{
  Serial.printf("Packet content received:%s\n", data);
}

void Node::get_routing_node_address() 
{
  //Set the routing table list that is being used and cannot be accessed (Remember to release use after usage)
  memset(this->_dsp_node_addr,0, LORA_MAX_NODE_ADDRESS*sizeof(uint16_t));
  int j = 0;
  for (int i = 0; i < MAX_ADDRESS_NODE; i++)
  {
    if (Node1._node_slot[i])
    {
      Serial.printf("Node %x with timeout count to %d\n", Node1._node_array[i].addr, Node1._node_array[i].timeout_counting);
      this->_dsp_node_addr[j] = Node1._node_array[i].addr;
      j++;
    }
  }
}

// char temp_received_msg_buff[100];

void Node::handle_process_received_message()
{
  /* Wait for the notification of processReceivedPackets and enter blocking */
  ulTaskNotifyTake(pdPASS, portMAX_DELAY);

  //Iterate through all the packets inside the Received User Packets Queue
  while (!User_receive_queue.is_empty())
  {
    Serial.println("ReceivedUserData_TaskHandle notify received");
    memset(_LoRa_buffer, 0, MAX_BUFFER_LENGTH);
    User_receive_queue.get_payload_head(this->_LoRa_buffer);
    int pay_load_pos = get_index_from_string(this->_LoRa_buffer, '&', PACKET_PAYLOAD_POSITION-1) + 1;
    // Serial.printf("Packet received, content is: %s\n",buffer+pos+1);
    User_receive_queue.delete_head();
    Serial.println(this->_LoRa_buffer+pay_load_pos);

    if (str_startswith(this->_LoRa_buffer + pay_load_pos, "MFSN")) // Message For Sink Node : message that users send to sink node
    {
      int msg = LORA_NODE_DATA_SIGNAL;
      // memset(_LoRa_buffer, 0, 250);
      // strcpy(_LoRa_buffer, _pre_LoRa_buffer + pay_load_pos+5);
      // Serial.println(_LoRa_buffer);

      if (xQueueSend(this->_cmd_msg_queue, (void *)&msg, 100) != pdTRUE)
      {
        Serial.println("From LoRa: CMD event queue is full");
      }
      vTaskDelay(20);
    }
    else if (str_startswith(this->_LoRa_buffer+pay_load_pos, "MFAN")) // Message For Actuator Node : message that sink node send to actuator node
    {
      Serial.println("A data from sink node sends to actuator node:");
      int actuator_payload_pos = get_index_from_string(this->_LoRa_buffer, ',', 0) + 1;
      Serial.println(this->_LoRa_buffer+actuator_payload_pos);
      Serial2.println(this->_LoRa_buffer+actuator_payload_pos);
    }
  }
}

