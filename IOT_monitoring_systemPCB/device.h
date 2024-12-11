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
#define DATA_REMAIN_FILE_PATH         "/data_sensor_remain.txt"
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
  char current_page_flag;
  uint8_t current_data_signal;
  uint8_t data_sensor_remaining_flag;

} Device_flags;

typedef struct
{
  volatile float    temp_data;
  volatile float    humid_data;
  volatile int      co2_data;
  volatile int      ch4_data; 
  volatile float      n2o_data; 
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
    /**
     * @brief Buffer to store data from other task
     */
    char                 _buffer[SERIAL_INPUT_MAX_BUFFER]; // handle command serial
    char                 _setting_buffer[SETTING_MAX_BUFFER]; // store setting data before storing to SD database
    
    char                 _serial_buffer[SETTING_MAX_BUFFER]; // store setting data before storing to SD database
    char                 _buttons_buffer[SETTING_MAX_BUFFER]; // store setting data before storing to SD database
    char                 _display_buffer[SETTING_MAX_BUFFER]; // store setting data before storing to SD database
    char                 _LoRa_node_buffer[SETTING_MAX_BUFFER]; // store setting data before storing to SD database
    char                 _database_buffer[SETTING_MAX_BUFFER]; // store setting data before storing to SD database
    char                 _dsp_bucket_buffer[SETTING_MAX_BUFFER]; // store setting data before storing to SD database
    char                 _sensor_buffer[SENSOR_MAX_BUFFER];
    
    Sensor_data_bucket_t _sensors_data_bucket;
    
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
    TimerHandle_t         _db_wait_timeout_timer = NULL;
    TimerHandle_t         _db_delay_timer = NULL;
    // QueueHandle_t        _cmd_msg_queue;
    // QueueHandle_t        _cmd_msg_queue;
        /**
     * @brief update state of the device: ON, OFF
     *
     * @return none
     */
    void display_update_device_state();
    void display_update_event_time();
    void display_update_measure_time(Measure_time_t *m_time, int time_type);
    void display_print_date_time();

  public:
      IOT_sensors          _sensors;
    IOT_device();
    IOT_device(int pump_operating_time, 
              int measure_time, 
              long int pressure_limit, 
              uint16_t discharge_valve2_time, 
              uint8_t sample_mode,
              int sensor_time_measure_interval,
              int sensor_sampling_number);
    void (*callbackfnc)();
    /**
     * @brief Begin the device
     *
     * @return zero if everything is ok, other numbers if something wrong
     */
    int         begin();
    void        init_event_time();
    void        init_event_time(Event_time_t *time_event);
    void        init_measure_event_time();
    void        init_measure_event_time(Measure_event_time_t *measure_event);
    int         run();
    int         on();
    void        off();
    void        sample();
    void        set_device_state(int state);
    int         get_device_state();
    int         get_device_on_state();
    friend void _on_timer();
    int         timer_setup(int timer, int prescaler, bool counter, void (*on_timer)());
    void        timer_start();
    void        timer_stop();
    void        timer_reset();
    int         get_timer_counter();
    void        reset_timer_counter();

    bool        check_is_measure_event_start();
    void        reset_measure_event_params();
    void        handle_device_on_error_event_state();
    int         save_data_to_database();
    void        calculate_sensor_data();
    void        collect_sensor_data();
    void        reset_sensor_data();

    int         add_event_time(DATE_t *event_time, int position);
    int         check_is_valid_event_time(DATE_t *event_time);
    int         check_is_valid_major_time(Measure_time_t *major_time);
    int         check_is_valid_minor_time(Measure_time_t *minor_time);
    int         check_is_valid_spike_time(Measure_time_t *spike_time);
    void        reset_default_time_event_array();
    uint8_t     handle_get_measure_event_time(int type);
    void        print_measure_time_event();
    int         get_node_role();
    int         set_node_role(int role);
    void        handle_get_node_role();
    void        handle_print_data_bucket(int type);

    int         create_task(TaskFunction_t pv_task_code, 
                            const char * name, 
                            const unsigned int stack_size, 
                            void *const param, 
                            UBaseType_t priority, 
                            TaskHandle_t *const task_handler, 
                            const int core);
    // void display_task(void *parameter);

    void        handle_command_serial();
    int         handle_cmd();
    int         process_cmd();
    void        handle_buttons_input();
    void        handle_get_params(String &params, uint8_t indicator);
    void        handle_save_to_database();
    void        handle_set_date_time();
    void        handle_save_setting();
    uint8_t     handle_load_setting_from_db();
    // void        handle_load_setting_from_database();
    void        handle_get_data_from_display();
    void        handle_data_from_transceiver();

    // actuator utilities
    void        set_chamber_door(int state);
    // display utilities
    void        send_sensor_data_to_display();
    
};

#endif
