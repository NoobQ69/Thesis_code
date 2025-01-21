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
#ifndef IOT_MONITORING_SYSTEM_UTILITIES_H_
#define IOT_MONITORING_SYSTEM_UTILITIES_H_

#define ENABLE                                    1
#define DISABLE                                   0
#define ON                                        1
#define OFF                                       0
#define SET                                       1
#define RESET                                     0
#define VALID                                     ENABLE
#define INVALID                                   DISABLE

/* TASK BUFFER MACROS */
#define CMD_MAX_BUFFER                            100
#define SERIAL_INPUT_MAX_BUFFER                   100
#define SETTING_MAX_BUFFER                        100
#define SENSOR_MAX_BUFFER                         100
#define DISPLAY_MAX_BUFFER                        100
#define DATABASE_MAX_BUFFER                       100
#define LORA_MAX_BUFFER                           100
#define BUTTON_MAX_BUFFER                         100

#define DEVICE_STATE_ON                           0
#define DEVICE_STATE_OFF                          1
#define DEVICE_STATE_SAMPLE                       2
#define DEVICE_STATE_FLOAT                        3

#define EVENT_TIME_RANGE                          24
#define ME_TIME_DEFAULT_HOURS                     8
#define ME_TIME_DEFAULT_MINUTES                   0

/*
  MEASURE EVENT*/
enum 
{
  EVENT_TIME_ADD,
  EVENT_TIME_UPDATE,
  EVENT_TIME_DELETE,
  EVENT_TIME_DELETE_TAIL,
  MAJOR_TIME,
  MINOR_TIME,
  SPIKE_TIME,

};

// EVENT SIGNAL MESSAGE QUEUE
enum
{
  NO_DATA_SIGNAL,
  SERIAL_DATA_SIGNAL,
  BUTTONS_DATA_SIGNAL,
  DISPLAY_DATA_SIGNAL,
  LORA_NODE_DATA_SIGNAL,
  DATABASE_DATA_SIGNAL,
  SAVE_DATA_SENSOR_SIGNAL,
  DATABASE_SEND_DATA_TIMEOUT_SIGNAL,
  DATABASE_SUCCESS_SEND_DATA_SIGNAL,
  DELAY_TIMEOUT_SIGNAL,
};

// DEVICE ROLE
enum
{
  NODE_ACTUATOR_STATION,
  NODE_ACTUATOR_DEVICE,
  NODE_SINK,
  NODE_GATEWAY,
  NODE_USER,
};

/*
  DEVICE FLAGS
*/
#define ON_EVENT_ERROR_NONE                      0
#define ON_EVENT_ERROR_DOOR_NOT_CLOSED           1

/*
  DEVICE STATE
*/

enum 
{
  DEVICE_ON_IDLE,
  DEVICE_ON_START_MEASURE_EVENT,
  DEVICE_ON_END_MEASURE_EVENT,
  DEVICE_ON_ERROR,
  DEVICE_ON_WAITING_DOOR,
  DEVICE_ON_MAJOR_TIME,
  DEVICE_ON_MINOR_TIME,
  DEVICE_ON_SPIKE_TIME,
  DEVICE_ON_GET_SENSOR_VALUE,
  DEVICE_ON_FAN_STILL_TIME,
};

/* MACROS STATE CODE FOR CLASS FUNCTIONS

  0x3210
  num 0, 1: error code
  num 2, 3: identity between classes 

 */

/* TASK MACROS */
#define DEVICE_SUCCESS_INIT_DISPLAY_TASK          0x0010
#define DEVICE_ERR_INIT_DISPLAY_TASK              0x0011


/* SENSORS MACROS */
#define SENSOR_INIT_SUCCESS                       0x0001
#define SENSOR_INIT_WITH_TROUBLE                  0x0002

#define SENSOR_DHT30_SUCCESS_INIT                 0x0003
#define SENSOR_DHT30_SUCCESS_READ_DATA            0x0004
#define SENSOR_DHT30_ERR_NOT_FOUND                0x0006
#define SENSOR_DHT30_ERR_NOT_INIT                 0x0007
#define SENSOR_DHT30_ERR_READ_TEMP_FAILED         0x0008
#define SENSOR_DHT30_ERR_READ_HUMID_FAILED        0x0009
#define SENSOR_DHT30_ERR_READ_DATA_FAILED         0x0010
#define SENSOR_BMP180_SUCCESS_INIT                0x0011
#define SENSOR_BMP180_ERR_INIT_FAILED             0x0012
#define SENSOR_BMP180_NOT_INIT                    0x0013

#define SENSOR_GET_CO2      0
#define SENSOR_GET_CH4      1
#define SENSOR_GET_ALL      2

#define SENSOR_RS485_BAUDRATE 4800

/* DATABASE MACROS */
#define DATABASE_SUCCESS_INIT                     0x0100
#define DATABASE_ERROR_INIT                       0x0101

#define DATABASE_SUCCESS_OPEN_DIRECTORY           0x0102
#define DATABASE_SUCCESS_CREATE_DIRECTORY         0x0103
#define DATABASE_SUCCESS_REMOVE_DIRECTORY         0x0104
#define DATABASE_SUCCESS_READ_FILE                0x0105
#define DATABASE_SUCCESS_WRITE_FILE               0x0106
#define DATABASE_SUCCESS_RENAME_FILE              0x0107
#define DATABASE_SUCCESS_DELETE_FILE              0x0108

#define DATABASE_ERROR_OPEN_DIRECTORY             0x0109
#define DATABASE_ERROR_NOT_A_DIRECTORY            0x010A
#define DATABASE_ERROR_CREATE_DIRECTORY           0x010B
#define DATABASE_ERROR_REMOVE_DIRECTORY           0x010C
#define DATABASE_ERROR_FAILED_OPEN_FILE           0x010D
#define DATABASE_ERROR_FAILED_READ_FILE           0x010E
#define DATABASE_ERROR_FAILED_WRITE_FILE          0x010F
#define DATABASE_ERROR_RENAME_FILE                0x0110
#define DATABASE_ERROR_DELETE_FILE                0x0111

/* need to add more macros ... */

/* INDICATOR SERIAL CMD */
#define CMD_SET_V2_DISCHARGE_TIME                 0
#define CMD_SET_MEASURE_TIME                      1
#define CMD_SET_P1_ON_TIME                        2
#define CMD_SET_PRESURE_LIMIT                     3
#define CMD_SET_SENSOR_TIME_MEASURE_INTERVAL      4
#define CMD_SET_SENSOR_SAMPLING_NUMBER            5

 /*
  ACTUATOR MACROS
  */
#define ACTUATORS_DOOR_CLOSED                       1
#define ACTUATORS_DOOR_OPEN                         0
#define ACTUATORS_TIMEOUT_CHAMBER_DOOR              5 // 5 sec

/*
 DISPLAY MACROS
 */
#define PAGE_MAX_NUMBER 7
#define DSP_ENTER 0
#define DSP_UP 1
#define DSP_DOWN 2

// /* SAMPLE MODE DEFINITION */
// #define SAMPLE_MODE_SINGLE                        0
// #define SAMPLE_MODE_MANY                          1

int map_value(int value, int fromLow, int fromHigh, int toLow, int toHigh);
int str_startswith(const char * string_compare,  const char * string_to_compare);
void split_string_char(const char *data, char separator, int index, char string_to_store[]);
int convert_string_to_int(const char *string_number, int len);
void convert_int_to_str(int N, char *str);
int count_character(const char *string,int len, char char_indicator);
int get_index_from_string(const char *str, char c, int position); // get index of a character from a string 

#endif