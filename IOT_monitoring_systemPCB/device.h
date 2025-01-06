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
#ifndef IOT_MONITORING_SYSTEM_DEVICE_H
#define IOT_MONITORING_SYSTEM_DEVICE_H

#include <stdint.h>
#include "utilities.h"
#include "buttons.h"
#include "actuators.h"
#include "sensors.h"
#include "display.h"
#include "database.h"
#include "rtc.h"
#include "gps.h"

#define SENSOR_RECORD_FILE_PATH   "/sensor_record.txt"
#define SETTING_FILE_PATH         "/setting_file.txt"
#define DATA_REMAIN_FILE_PATH     "/data_sensor_remain.txt"
#define BUZZER_PIN                2

typedef struct 
{
  uint16_t time;
  uint16_t number_of_intervals;

}  Measure_time_t;

typedef struct 
{
  uint8_t number_of_event_time;
  DATE_t  time[EVENT_TIME_RANGE];

} Event_time_t;

typedef struct
{
  Event_time_t    event_time; 
  Measure_time_t  major_time; // in minutes
  Measure_time_t  minor_time; // in minutes
  Measure_time_t  spike_time; // in seconds
  Measure_time_t  fan_still_time; // in seconds

} Measure_event_time_t;

typedef struct
{
  uint8_t forcing_to_event_flag;
  uint8_t major_time_on_flag;
  uint8_t minor_time_on_flag;
  uint8_t spike_time_on_flag;
  uint8_t on_event_error_flag; // in seconds

  uint8_t chamber_door_state_flag;
  uint8_t fan_1_state_flag;
  uint8_t fan_2_state_flag;
  uint8_t pump_1_state_flag;
  char    current_page_flag;
  uint8_t current_data_signal;
  uint8_t data_sensor_remaining_flag;

} Device_flags;

typedef struct
{
  volatile float    temp_data;
  volatile float    humid_data;
  volatile int      co2_data;
  volatile int      ch4_data; 
  volatile float    n2o_data; 
  volatile int      pressure_data;

} Sensor_data_bucket_t;

/**
 * @brief IOT device class, this class will responsible for handling all component of the device
 *
 */
class IOT_device
{
  private:
    /**
     * @brief Component class of the device
     */

    IOT_buttons          _buttons;
    IOT_actuators        _actuators;
    IOT_database         _database;
    IOT_rtc              _rtc;
    IOT_display          _display;
    IOT_GPS              _gps;
    Device_flags         _device_flags;
    /**
     * @brief Event time variable for measuring procedure, for more information, check: 
     */
    Measure_event_time_t _measure_events;
    uint16_t             _timeout_chamber_door;
    Measure_time_t       _setpoint_measure_major_time;
    Measure_time_t       _setpoint_measure_minor_time;
    Measure_time_t       _setpoint_measure_spike_time;
    Measure_time_t       _setpoint_chamber_door_time;
    Measure_time_t       _setpoint_fan_still_time;

    hw_timer_t           *_my_timer;
    Sensor_data_bucket_t _sensors_data_bucket;
    Sensor_data_bucket_t _dsp_sensors_data_bucket;
    
    int                  _device_state;
    int                  _device_on_state;
    int                  _device_role;
    // for scroll up and down display data;
    int                  _dsp_current_line_bucket;
    int                  _sensor_file_num_of_lines;
    int                  _sensor_data_remain;
    int                  _sensor_data_len;
    
    QueueHandle_t        _store_db_msg_queue;
    QueueHandle_t        _LoRa_msg_queue;
    QueueHandle_t        _cmd_msg_queue;
    SemaphoreHandle_t    _database_mutex;
    TimerHandle_t        _db_wait_timeout_timer = NULL;
    TimerHandle_t        _db_delay_timer = NULL;

    /**
      * @brief Buffers to store data from other tasks
      */
    
    char                 _buffer[CMD_MAX_BUFFER]; // handle command serial
    char                 _setting_buffer[SETTING_MAX_BUFFER]; // store setting data before storing to SD database
    
    char                 _serial_buffer[SERIAL_INPUT_MAX_BUFFER]; // store setting data before storing to SD database
    char                 _buttons_buffer[BUTTON_MAX_BUFFER]; // store setting data before storing to SD database
    char                 _display_buffer[DISPLAY_MAX_BUFFER]; // store setting data before storing to SD database
    char                 _LoRa_node_buffer[LORA_MAX_BUFFER]; // store setting data before storing to SD database
    char                 _database_buffer[DATABASE_MAX_BUFFER]; // store setting data before storing to SD database
    char                 _dsp_bucket_buffer[DISPLAY_MAX_BUFFER]; // store setting data before storing to SD database
    char                 _sensor_buffer[SENSOR_MAX_BUFFER];
    

    // QueueHandle_t        _cmd_msg_queue;
    // QueueHandle_t        _cmd_msg_queue;
   /**
     * @brief update state of the device: ON, OFF
     *
     * @return none
     */
    void display_update_device_state();
    
    /**
     * @brief update event time to display
     *
     * @return none
     */
     void display_update_event_time();

    /**
     * @brief update measure time to display
     *
     * @return none
     */
    void display_update_measure_time(Measure_time_t *m_time, int time_type);
    
   /**
     * @brief update time to display periodically
     *
     * @return none
     */
    void display_print_date_time();

  public:
    /**
     * @brief instant of sensor class, need to be in public to deal with problem when reading Serial using SoftwareSerial, for more
     *        information, check IOT_monitoring_systemPCB.ino
     */
    IOT_sensors          _sensors;
    /**
     * @brief function pointer for dealing with problem when reading Serial using SoftwareSerial, for more
     *        information, check IOT_monitoring_systemPCB.ino
     */
    void (*callbackfnc)();
    /**
     * @brief constructor of the class
     */
    IOT_device();
    /**
     * @brief constructor of the class
     * 
     * @param the paramters of the constructor are deprecated, need to adjust 
     *
     * @return none
     */
    IOT_device(int pump_operating_time, 
              int measure_time, 
              long int pressure_limit, 
              uint16_t discharge_valve2_time, 
              uint8_t sample_mode,
              int sensor_time_measure_interval,
              int sensor_sampling_number);
    
    
    /**
     * @brief Begin the device
     *
     * @return zero if everything is ok, other numbers if something wrong, the return value need to return code that is defined in utilities.h file
     */
    int         begin();

   /**
     * @brief Init event time setup, for more information about this, check sampling procedure in :
     *
     * @return None
     */
    void        init_event_time();

    /**
     * @brief Init event time setup, for more information about this, check sampling procedure in :
     *
     * @param Event time struct variable
     *
     * @return None
     */
    void        init_event_time(Event_time_t *time_event);

   /**
     * @brief Init measure time setup, for more information about this, check sampling procedure in :
     *
     * @return None
     */
    void        init_measure_event_time();

    /**
     * @brief Init event time setup, for more information about this, check sampling procedure in :
     *
     *
     * @param Measure time struct variable
     *
     * @return None
     */
    void        init_measure_event_time(Measure_event_time_t *measure_event);

   /**
     * @brief run the code that handle measurement of the device
     *
     * @return 0
     */
    int         run();

   /**
     * @brief run the sampling measurement when time events are reached or the forcing_to_event_flag flag set
     *
     * @return 0
     */
    int         on();

   /**
     * @brief off the sampling measurement when the procedure is done
     *
     * @return None
     */
    void        off();

   /**
     * @brief this function is deprecated, need to adjust or delete
     *
     * @return None
     */
    void        sample();

   /**
     * @brief Set state on or off or sample of the device
     *
     * @return None
     */
    void        set_device_state(int state);
    
   /**
     * @brief Get state on or off or sample of the device
     *
     * @return None
     */
    int         get_device_state();

   /**
     * @brief This function is deprecated, need to adjust or delete
     *
     * @return None
     */
    int         get_device_on_state();

   /**
     * @brief Friend function to count tick of the timer 0 in esp32
     *
     * @return None
     */
    friend void _on_timer();

   /**
     * @brief Set up hardware timer of esp32 mcu
     *
     * @param timer : timer number, from 0 to 3 
     * @param prescaler : timer prescaler 
     * @param counter : timer counter, decide whether timer is onshot or repeated 
     * @param counter : timer callback function, user write code to handle when timer event happeneded
     *
     * @return 0
     */
    int         timer_setup(int timer, int prescaler, bool counter, void (*on_timer)());

   /**
     * @brief Start timer counter
     *
     * @return None
     */
    void        timer_start();

   /**
     * @brief Stop timer counter
     *
     * @return None
     */
    void        timer_stop();

   /**
     * @brief Reset timer counter
     *
     * @return None
     */
    void        timer_reset();

   /**
     * @brief Get current tick timer counter
     *
     * @return tick, need to adjust because global variable that counts tick is long long int type
     */
    int         get_timer_counter();

   /**
     * @brief Reset current tick timer counter
     *
     * @return None
     */
    void        reset_timer_counter();
    
   /**
     * @brief Check event time is reached or not
     *
     * @return true if time event is reached, false if not
     */
    bool        check_is_measure_event_started();

   /**
     * @brief Reset measure event time, reset all value and set default value to 8:00 am
     *
     * @return None
     */
    void        reset_measure_event_params();

   /**
     * @brief Handle when measurement procedure is failed or something wrong happended
     *
     * @return None
     */
    void        handle_device_on_error_event_state();

   /**
     * @brief Save sensor data to database
     *
     * @return true if value is saved, false if not
     */
    int         save_data_to_database();

   /**
     * @brief Calculate average sensor value measurement 
     *
     * @return None
     */
    void        calculate_sensor_data();
   /**
     * @brief Accumulate sensor value measurement 
     *
     * @return None
     */
    void        collect_sensor_data();
   /**
     * @brief Reset sensor value in _sensors_data_bucket variable 
     *
     * @return None
     */
    void        reset_sensor_data();
   /**
     * @brief add or update event time
     *
     * @param event_time : event time to add
     * @param position : in updating case, the position is used to indicate where to update event time
     *
     * @return result of adding event time
     */
    int         add_event_time(DATE_t *event_time, int position);
   /**
     * @brief check if event time that users added is valid or not
     *
     * @param event_time : event time to check
     *
     * @return result of adding event time
     */
    int         check_is_valid_event_time(DATE_t *event_time);
   /**
     * @brief check if major time that users added is valid or not
     *
     * @param major_time : major time to check
     *
     * @return result of adding major time
     */
    int         check_is_valid_major_time(Measure_time_t *major_time);
   /**
     * @brief check if minor time that users added is valid or not
     *
     * @param minor_time : minor time to check
     *
     * @return result of adding minor time
     */
    int         check_is_valid_minor_time(Measure_time_t *minor_time);
   /**
     * @brief check if spike time that users added is valid or not
     *
     * @param spike_time : spike time to check
     *
     * @return result of adding spike time
     */
    int         check_is_valid_spike_time(Measure_time_t *spike_time);
   /**
     * @brief reset event time to default, in default setting, there is only one event time at 8:00 am stored
     *
     * @return None
     */
    void        reset_default_time_event_array();
   /**
     * @brief handle get measure time such as major time, minor time, spike time,
     *
     * @param type : type to handle such as major time, minor time, spike time, if time is valid, the time will be updated
     *
     * @return result of handling measure times
     */
    uint8_t     handle_get_measure_event_time(int type);
   /**
     * @brief print event time and measure time
     *
     * @return None
     */
    void        print_measure_time_event();
   /**
     * @brief get node role
     *
     * @return node role in number, defined as macros in utitilies.h file
     */
    int         get_node_role();
   /**
     * @brief set node role
     *
     *@param role : node role in number, defined as macros in utitilies.h file
     *
     * @return 0 (note: this return is deprecated)
     */
    int         set_node_role(int role);
   /**
     * @brief handle process node role from user or other components
     *
     * @return None
     */
    void        handle_get_node_role();
   /**
     * @brief handle print data bucket to user via serial or display
     *
     * @param type : the type indicate the number of current line to print to user, type is defined at DISPLAY MACROS in utilities.h file  
     *
     * @return None
     */
    void        handle_print_data_bucket(int type);
   /**
     * @brief create task that handle operations in the class object
     *
      * @param pv_task_code : Function to be called
      * @param name : Name of task
      * @param stack_size : Stack size (bytes in ESP32, word in FreeRTOS)
      * @param param : parameter to pass function
      * @param priority : Task priority ( 0 to configMAX_PRIORITIES - 1)
      * @param task_handler : Task handle
      * @param core : Run on specified core  
     *
     * @return None
     */
    int         create_task(TaskFunction_t pv_task_code, 
                            const char * name, 
                            const unsigned int stack_size, 
                            void *const param, 
                            UBaseType_t priority, 
                            TaskHandle_t *const task_handler, 
                            const int core);
    // void display_task(void *parameter);
   /**
     * @brief handle serial data from user
     *
     * @return None
     */
    void        handle_command_serial();
   /**
     * @brief handle command line from users or other components that is sended via tasks
     *
     * @return 0 (this return value is deprecated)
     */
    int         handle_cmd();
   /**
     * @brief process command line from users or other components that is sended via tasks
     *
     * @return 0 (this return value is deprecated)
     */
    int         process_cmd();
   /**
     * @brief get button signal from users that is serverd via input_task
     *
     * @return None
     */
    void        handle_buttons_input();
   /**
     * @brief handle save sensor data to database, the function name is ambiguous and needed to rename 
     *
     * @return None
     */
    void        handle_save_to_database();
   /**
     * @brief handle set date time to device 
     *
     * @return None
     */
    void        handle_set_date_time();
   /**
     * @brief handle save setting data to SD card, setting is event time, measure time  
     *
     * @return None
     */
    void        handle_save_setting();
   /**
     * @brief handle load setting data from SD card, setting is event time, measure time  
     *
     * @return 0 (the return value is deprecated)
     */
    uint8_t     handle_load_setting_from_db();
   /**
     * @brief handle get data from display that is served from input_task
     *
     * @return None
     */
    void        handle_get_data_from_display();
   /**
     * @brief handle get data from Lora node that is served from LoRa_transceiver_task
     *
     * @return None
     */
    void        handle_data_from_transceiver();

    // actuator utilities
    void        set_chamber_door(int state);
   /**
     * @brief send current data sensor to display
     *
     * @return None
     */
    void        send_sensor_data_to_display();
    
};

#endif
