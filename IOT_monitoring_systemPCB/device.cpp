#include "Arduino.h"
#include "freertos/portmacro.h"
#include "SD.h"
#include "esp_attr.h"
#include "device.h"

QueueHandle_t        _store_db_msg_queue;
int db_delay_timeout_flag = 0;
int db_send_data_timeout_flag = 0;

// char Dsp_ET_current_label[][10] = {"b4.txt=\"","b5.txt=\"","b6.txt=\"","b7.txt=\""};
char Dsp_MT_current_label[][8] = {"n0.val=","n1.val=","n2.val=","n3.val=","n4.val=","n5.val="};
char Event_time_string_temp[20];

long long int Timer_counter = 0;

void IOT_device::set_chamber_door(int state)
{
  if (state == ON)
  {
    this->_actuators.set_actuators_pin(this->_actuators.get_cylinder_1_pins(), HIGH);
    this->_actuators.set_actuators_pin(this->_actuators.get_cylinder_2_pins(), LOW);
  }
  else
  {
    this->_actuators.set_actuators_pin(this->_actuators.get_cylinder_1_pins(), LOW);
    this->_actuators.set_actuators_pin(this->_actuators.get_cylinder_2_pins(), HIGH);
  }
}

void IOT_device::send_sensor_data_to_display()
{
  String cmd_display = "";
  cmd_display = "co2_label.txt=\"";
  cmd_display += String(this->_sensors.get_data_co2());
  cmd_display += "\"";
  this->_display.send_data_to_display(cmd_display.c_str());
  cmd_display = "no2_label.txt=\"";
  cmd_display += String(this->_sensors.get_data_no2());
  cmd_display += "\"";
  this->_display.send_data_to_display(cmd_display.c_str());
  cmd_display = "ch4_label.txt=\"";
  cmd_display += String(this->_sensors.get_data_ch4());
  cmd_display += "\"";
  this->_display.send_data_to_display(cmd_display.c_str());
  cmd_display = "temp_label.txt=\"";
  cmd_display += String(this->_sensors.get_data_temp_humid(0));
  cmd_display += "\"";
  this->_display.send_data_to_display(cmd_display.c_str());
  cmd_display = "humid_label.txt=\"";
  cmd_display += String(this->_sensors.get_data_temp_humid(1));
  cmd_display += "\"";
  this->_display.send_data_to_display(cmd_display.c_str());
  cmd_display = "p_label.txt=\"";
  cmd_display += String(this->_sensors.get_data_pressure());
  cmd_display += "\"";
  this->_display.send_data_to_display(cmd_display.c_str());
}

static void on_buzzer(int delay_ms = 200)
{
  digitalWrite(BUZZER_PIN, HIGH);
  vTaskDelay(delay_ms/portTICK_PERIOD_MS);
  digitalWrite(BUZZER_PIN, LOW);
}

static void IRAM_ATTR _on_iot_device_timer_isr()
{
  Timer_counter++;
}

// int signal_timer;
static void my_timer_callback(TimerHandle_t xTimer) 
{ 
  if ((uint32_t)pvTimerGetTimerID(xTimer) == 0)
  {
    db_delay_timeout_flag = 1;
    // xQueueSendFromISR( _store_db_msg_queue, (void *)&signal_timer, &xHigherPriorityTaskWoken );
  }
  else if ((uint32_t)pvTimerGetTimerID(xTimer) == 1)
  {
    db_send_data_timeout_flag = 1;
  }
}

/* Default setting*/
IOT_device::IOT_device()
{
  // Init variables
  // this->init_measure_event_time();                      // default setting
  this->_my_timer                           = NULL;
  // this->_sensor_buffer                      = "";
  this->_timeout_chamber_door               = ACTUATORS_TIMEOUT_CHAMBER_DOOR;
  memset(this->_buffer, 0, SERIAL_INPUT_MAX_BUFFER);
  _store_db_msg_queue = xQueueCreate(3, sizeof(int));
  this->_LoRa_msg_queue = xQueueCreate(2, sizeof(int));
  this->_cmd_msg_queue = xQueueCreate(10, sizeof(int));
  this->_database_mutex = xSemaphoreCreateMutex();
  this->_db_wait_timeout_timer = xTimerCreate(
                    "Sensor data timeout timer",           // Name of timer
                    30000 / portTICK_PERIOD_MS,  // Period of timer (in ticks)
                    pdFALSE,                    // Auto-reload
                    (void *)0,                  // Timer ID
                    my_timer_callback);           // Callback function;
  this->_db_delay_timer = xTimerCreate(
                    "None blocking delay timer",           // Name of timer
                    30000 / portTICK_PERIOD_MS,  // Period of timer (in ticks)
                    pdFALSE,                    // Auto-reload
                    (void *)1,                  // Timer ID
                    my_timer_callback);           // Callback function;
  // this->_timer_counter = 0;
  
  // Init flags
  this->_device_state                                     = DEVICE_STATE_FLOAT;
  this->_device_on_state                                  = DEVICE_ON_IDLE;
  this->_device_role                                      = NODE_ACTUATOR_DEVICE;
  this->_device_flags.forcing_to_event_flag               = DISABLE;
  this->_device_flags.major_time_on_flag                  = DISABLE;
  this->_device_flags.minor_time_on_flag                  = DISABLE;
  this->_device_flags.spike_time_on_flag                  = DISABLE;
  this->_device_flags.on_event_error_flag                 = ON_EVENT_ERROR_NONE;
  this->_device_flags.current_page_flag                   = 0;
  this->_device_flags.current_data_signal                 = NO_DATA_SIGNAL;
  this->_dsp_current_line_bucket                          = 0;
  this->_device_flags.data_sensor_remaining_flag          = RESET;

  this->_sensor_data_remain                               = 0;
  this->_sensor_data_len                                  = 0;

}

// IOT_device::IOT_device(int pump_operating_time            = 300, 
//                        int measure_time                   = 20, 
//                        long int pressure_limit            = 1024, 
//                        uint16_t discharge_valve2_time     = 5,
//                        uint8_t sample_mode                = SAMPLE_MODE_SINGLE,
//                        int sensor_time_measure_interval   = 200,
//                        int sensor_sampling_number         = 5
//                        )
// {

//   this->_my_timer               = NULL;
//   // this->_timer_counter = 0;
//   this->_device_state           = DEVICE_STATE_FLOAT;
// }

int IOT_device::begin()
{
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  this->_buttons.begin();
  this->_actuators.begin();
  if(this->_database.begin() == DATABASE_ERROR_INIT)
  {
    // pinMode(BUZZER_PIN, OUTPUT);
    pinMode(2, OUTPUT);

    while (1) // when database holding critical information and its init is failed, better to hang the program and notify to users
    {
      Serial.println("SD card failed to communicate, hanging...");
      // digitalWrite(BUZZER_PIN, HIGH);
      digitalWrite(2, HIGH);
      delay(300);
      digitalWrite(2, LOW);
      delay(5000);
    }

    this->init_measure_event_time();                      // obsoleted

  }
  else
  {
    this->handle_load_setting_from_db();
  }
  this->_rtc.begin();
  this->_sensors.begin();
  this->_display.begin();
  this->_display.print_page_on_display(-1);
  this->timer_setup(0, 80, true, &_on_iot_device_timer_isr);

  int temp = 0;
  if (this->_database.get_number_of_line(SD, SENSOR_RECORD_FILE_PATH, &this->_sensor_data_len) == DATABASE_ERROR_FAILED_OPEN_FILE)
  {
    this->_sensor_data_len = 0;
  }
  if (this->_database.read_file(SD, DATA_REMAIN_FILE_PATH, this->_buffer, &temp) == DATABASE_ERROR_FAILED_OPEN_FILE)
  {
    this->_database.write_file(SD, DATA_REMAIN_FILE_PATH, "0", false);
  }
  _sensor_data_remain = (int)strtol(this->_buffer, NULL, 10);
  Serial.printf("SS remain line: %d",_sensor_data_remain);
  Serial.printf("SS line: %d",_sensor_data_len);
  if (_sensor_data_remain <= this->_sensor_data_len)
  {
    this->_device_flags.data_sensor_remaining_flag = SET;
  }
  if (_sensor_data_remain > this->_sensor_data_len)
  {
    this->_device_flags.data_sensor_remaining_flag = SET;
    while (_sensor_data_remain >= this->_sensor_data_len)
    {
      _sensor_data_remain -= 5;
      if (_sensor_data_remain <= 0) {_sensor_data_remain = 0; break;}
    }
  }
  // for (int i = 0; i < 3; i++)
  // {
  //   on_buzzer();
  // }
  // this->_database.write_file(SD, "/log.txt","log", true);
  return 0;
}

void IOT_device::init_event_time()
{
  this->_measure_events.event_time.number_of_event_time = 1;
  int l = this->_measure_events.event_time.number_of_event_time;

  this->_measure_events.event_time.time[0].hours    = 8;
  this->_measure_events.event_time.time[0].minutes  = 0;
  this->_measure_events.event_time.time[0].seconds  = 0;
}

void IOT_device::init_event_time(Event_time_t *time_event)
{
  this->_measure_events.event_time.number_of_event_time = time_event->number_of_event_time;
  int l = this->_measure_events.event_time.number_of_event_time;
  
  for (int i = 0; i < l; i++)
  {
    this->_measure_events.event_time.time[i].hours    = time_event->time[i].hours;
    this->_measure_events.event_time.time[i].minutes  = time_event->time[i].minutes;
    this->_measure_events.event_time.time[i].seconds  = time_event->time[i].seconds;
  }
}

void IOT_device::init_measure_event_time()
{
  this->init_event_time();

  this->_measure_events.major_time.time                 = 3; // 60 minutes
  this->_measure_events.major_time.number_of_intervals  = 3; // 3 times

  this->_measure_events.minor_time.time                 = 1; // 15 minutes
  this->_measure_events.minor_time.number_of_intervals  = 2; // 3 times -> 15*3 = 45 minutes < 60 minute -> valid value setup
  
  this->_measure_events.spike_time.time                 = 5; // 15 seconds 
  this->_measure_events.spike_time.number_of_intervals  = 5; // 5 times, 15 * 5 = 75 seconds < 15 minutes -> valid value setup
  
  this->_measure_events.fan_still_time.time                 = 75; // 75 seconds 
}

void IOT_device::init_measure_event_time(Measure_event_time_t *measure_event)
{
  this->init_event_time(&measure_event->event_time);

  this->_measure_events.major_time.time                 = measure_event->major_time.time;
  this->_measure_events.major_time.number_of_intervals  = measure_event->major_time.number_of_intervals;

  this->_measure_events.minor_time.time                 = measure_event->minor_time.time;
  this->_measure_events.minor_time.number_of_intervals  = measure_event->minor_time.number_of_intervals;
  
  this->_measure_events.spike_time.time                 = measure_event->spike_time.time;
  this->_measure_events.spike_time.number_of_intervals  = measure_event->spike_time.number_of_intervals;
}

int IOT_device::create_task(TaskFunction_t pv_task_code, 
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

int IOT_device::run()
{
  if (this->_device_state == DEVICE_STATE_ON)
  {
    this->on();
    // this->_device_state = DEVICE_STATE_FLOAT;
  }
  else if (this->_device_state == DEVICE_STATE_OFF)
  {
    this->off();
    this->_device_state = DEVICE_STATE_FLOAT;
  }
  else if (this->_device_state == DEVICE_STATE_SAMPLE)
  {
    this->sample();
  }
  return 0;
}

void IOT_device::reset_measure_event_params()
{
  this->timer_stop();
  this->timer_reset();
  this->reset_timer_counter();
  this->_actuators.set_actuators_pin(FAN_1_PIN, LOW);
  this->_actuators.set_actuators_pin(CYLINDER_1_PIN, LOW);
  this->_actuators.set_actuators_pin(CYLINDER_2_PIN, LOW);
  // this->_actuators.set_actuators_pin(FAN_A_PIN, LOW);
  // this->_actuators.set_actuators_pin(PUMP_1_PIN, LOW);
  this->_device_flags.forcing_to_event_flag = DISABLE;
  this->_device_flags.major_time_on_flag    = DISABLE;
  this->_device_flags.minor_time_on_flag    = DISABLE;
  this->_device_flags.spike_time_on_flag    = DISABLE;
  this->_setpoint_measure_major_time.number_of_intervals = 0;
  this->_setpoint_measure_major_time.time = 0;
  this->_setpoint_measure_minor_time.number_of_intervals = 0;
  this->_setpoint_measure_minor_time.time = 0;
  this->_setpoint_measure_spike_time.number_of_intervals = 0;
  this->_setpoint_measure_spike_time.time = 0;
  this->_device_on_state = DEVICE_ON_IDLE;
}

bool IOT_device::check_is_measure_event_start()
{
  int len = this->_measure_events.event_time.number_of_event_time;
  for (int i = 0; i < len; i++)
  {
    if (this->_measure_events.event_time.time[i].hours == this->_rtc.get_hours())
    {
      if (this->_measure_events.event_time.time[i].minutes == this->_rtc.get_minutes())
      {
        return true;
      }
    }
  }
  return false;
}

void IOT_device::handle_device_on_error_event_state()
{
  if (this->_device_flags.on_event_error_flag == ON_EVENT_ERROR_DOOR_NOT_CLOSED)
  {
    // do something
  }
  else
  {
    // do something
  }
}

static int cmp_date_time(void * time_1, void * time_2)
{
  DATE_t *a = (DATE_t*)time_1;
  DATE_t *b = (DATE_t*)time_2;

  if (a->hours > b->hours)
  {
    return 1;
  }
  else if (a->hours == b->hours)
  {
    if (a->minutes > b->minutes) return 1;
  }

  return 0;
}

template <typename T>
static void sort_date_time_array(T arr[], int n, int (*cmp_callback_func)(void * a, void * b))
{
	int i = 0, j = 0;
  T key;

	for(i = 1;i < n; i++)
	{
		key = arr[i];
		j = i-1;
		while(j >= 0 && cmp_callback_func((void*)(&arr[j]), (void *)(&key)))
		{
			arr[j+1] = arr[j];
			j -= 1;
		}
		arr[j + 1] = key;
	}
}

template <typename T>
static int shift_left_array(T arr, int len, int position)
{
	for(int i = position;i < len; i++)
	{
    arr[i] = arr[i+1];
	}
  len -= 1;
  return len;
}

int IOT_device::add_event_time(DATE_t *event_time, int position = -1)
{
  int len = this->_measure_events.event_time.number_of_event_time;

  if (len >= EVENT_TIME_RANGE)
  {
    return INVALID;
  }

  if (position != -1)
  {
    this->_measure_events.event_time.time[position] = *event_time;
    // this->_measure_events.event_time.number_of_event_time += 1;
    sort_date_time_array(this->_measure_events.event_time.time, this->_measure_events.event_time.number_of_event_time,cmp_date_time);
    return VALID;
  }

  this->_measure_events.event_time.time[len] = *event_time;
  this->_measure_events.event_time.number_of_event_time += 1;
  sort_date_time_array(this->_measure_events.event_time.time, this->_measure_events.event_time.number_of_event_time,cmp_date_time);
  return VALID;
}

void IOT_device::reset_default_time_event_array()
{
  for (int i = 0; i < EVENT_TIME_RANGE; i++)
  {
    this->_measure_events.event_time.time[i].hours = 0;
    this->_measure_events.event_time.time[i].minutes = 0;
  }

  this->_measure_events.event_time.time[0].hours = ME_TIME_DEFAULT_HOURS;
  this->_measure_events.event_time.time[0].minutes = ME_TIME_DEFAULT_MINUTES;
  this->_measure_events.event_time.number_of_event_time = 1;
}

int IOT_device::check_is_valid_event_time(DATE_t *event_time)
{
  if (!(event_time->hours >= 0   && event_time->hours <= 24)) return INVALID;
  if (!(event_time->minutes >= 0 && event_time->minutes <= 59)) return INVALID;
  // if (!(event_time->seconds >= 0 && event_time->seconds <= 59)) return INVALID;
  
  return VALID;
}

int IOT_device::check_is_valid_major_time(Measure_time_t *major_time)
{
  int get_timespan_minutes = 0;
  int len = this->_measure_events.event_time.number_of_event_time;

  for (int i = 0;i < len-1; i++)
  {
    get_timespan_minutes = this->_measure_events.event_time.time[i+1].hours - this->_measure_events.event_time.time[i].hours;
    get_timespan_minutes *= 60;
    
    if ((major_time->time * major_time->number_of_intervals) > get_timespan_minutes) return INVALID;
  }

  return VALID;
}

int IOT_device::check_is_valid_minor_time(Measure_time_t *minor_time)
{
  // int get_timespan_minutes = 0;

  // get_timespan_minutes = this->_measure_events.major_time.time * this->_measure_events.major_time.number_of_intervals;
    
  if ((minor_time->time * minor_time->number_of_intervals) > this->_measure_events.major_time.time) return INVALID;

  return VALID;
}

int IOT_device::check_is_valid_spike_time(Measure_time_t *spike_time)
{
  int get_timespan_seconds = 0;

  get_timespan_seconds = this->_measure_events.minor_time.time * 60;
    
  if ((spike_time->time * spike_time->number_of_intervals + this->_measure_events.fan_still_time.time + 
      this->_timeout_chamber_door) > get_timespan_seconds) return INVALID;

  return VALID;
}

uint8_t IOT_device::handle_get_measure_event_time(int type)
{
  int result = INVALID;
  int params[3];
  char string_time_number[4];
  int len = this->_measure_events.event_time.number_of_event_time;
  DATE_t event_time = {0, 0, 0, 0, 0, 0};
  Measure_time_t sub_event_time;

  int num_of_param = count_character(this->_buffer,strlen(this->_buffer), ',');

  for (int i = 0; i < num_of_param; i++)
  {
    split_string_char(this->_buffer, ',', i+1, string_time_number);
    params[i] = convert_string_to_int(string_time_number, strlen(string_time_number));
    memset(string_time_number, 0, 4);
  }
  
  // split_string_char(this->_buffer, ',', 2, string_time_number);
  // params[1] = convert_string_to_int(string_time_number, strlen(string_time_number));
  
  if (type == EVENT_TIME_ADD)
  {
    event_time.hours = params[0];
    event_time.minutes = params[1];

    if (this->check_is_valid_event_time(&event_time) == VALID)
    {
      result = this->add_event_time(&event_time);
    }
    else
    {
      Serial.println("Error add event time: Invalid event time! Check input value");
      result = INVALID;
    }
  }
  else if (type == EVENT_TIME_UPDATE)
  {
    event_time.hours = params[1];
    event_time.minutes = params[2];

    if (this->check_is_valid_event_time(&event_time) == VALID)
    {
      result = this->add_event_time(&event_time, params[0]);
    }
    else
    {
      Serial.println("Error update event time: Invalid event time! Check input value");
      result = INVALID;
    }
  }
  else if (type == EVENT_TIME_DELETE)
  {
    this->_measure_events.event_time.number_of_event_time = shift_left_array(this->_measure_events.event_time.time, 
                                                            (int)this->_measure_events.event_time.number_of_event_time,
                                                            params[0]);
    result = VALID;
  }
  else if (type == EVENT_TIME_DELETE_TAIL)
  {
    this->_measure_events.event_time.time[this->_measure_events.event_time.number_of_event_time-1].hours = 0;
    this->_measure_events.event_time.time[this->_measure_events.event_time.number_of_event_time-1].minutes = 0;
    this->_measure_events.event_time.number_of_event_time -= 1;
    result = VALID;
  }
  else if (type == MAJOR_TIME)
  {
    sub_event_time.number_of_intervals = params[0];
    sub_event_time.time = params[1];

    if (this->check_is_valid_major_time(&sub_event_time) == VALID) 
    {
      this->_measure_events.major_time = sub_event_time;
      result = VALID;
    }
    else
    {
      Serial.println("Error major time: Invalid major time! Check input value");
      result = INVALID;
    }
  }
  else if (type == MINOR_TIME)
  {
    sub_event_time.number_of_intervals = params[0];
    sub_event_time.time = params[1];

    if (this->check_is_valid_minor_time(&sub_event_time) == VALID) 
    {
      this->_measure_events.minor_time = sub_event_time;
      result = VALID;
    }
    else
    {
      Serial.println("Error minor time: Invalid minor time! Check input value");
      result = INVALID;
    }
  }
  else if (type == SPIKE_TIME)
  {
    sub_event_time.number_of_intervals = params[0];
    sub_event_time.time = params[1];

    if (this->check_is_valid_spike_time(&sub_event_time) == VALID)
    {
      this->_measure_events.spike_time = sub_event_time;
      result = VALID;
    }
    else
    {
      Serial.println("Error spike time: Invalid spike time! Check input value");
      result = INVALID;
    }
  }

  if (type <= EVENT_TIME_DELETE_TAIL)
  {
    // for updating event time in display
    if (this->_device_flags.current_data_signal == DISPLAY_DATA_SIGNAL)
    {
      this->handle_save_setting();
      this->_display.send_data_to_display("page ev_time_page");
      if (result == VALID)
        this->_display.send_data_to_display("st_label.txt=\"OK\"");
      else
        this->_display.send_data_to_display("st_label.txt=\"ERROR\"");
    }
  }
  else
  {
    // for updating event time in display
    if (this->_device_flags.current_data_signal == DISPLAY_DATA_SIGNAL)
    {
      this->display_update_measure_time(&this->_measure_events.major_time, MAJOR_TIME);
      this->display_update_measure_time(&this->_measure_events.minor_time, MINOR_TIME);
      this->display_update_measure_time(&this->_measure_events.spike_time, SPIKE_TIME);
      this->handle_save_setting();
      this->_display.send_data_to_display("page ev_m_page");
      if (result == VALID)
        this->_display.send_data_to_display("st_label.txt=\"OK\"");
      else
        this->_display.send_data_to_display("st_label.txt=\"ERROR\"");
    }
  }

  return 0;
}

void IOT_device::print_measure_time_event()
{
  int len = this->_measure_events.event_time.number_of_event_time;
  Serial.println("Event time:");

  for (int i = 0; i < len; i++)
  {
    Serial.printf("%d:%d:%d\n", this->_measure_events.event_time.time[i].hours, this->_measure_events.event_time.time[i].minutes, this->_measure_events.event_time.time[i].seconds);
  }

  Serial.printf("Major time: time %d, no of itv %d\n", this->_measure_events.major_time.time, this->_measure_events.major_time.number_of_intervals);
  Serial.printf("Minor time: time %d, no of itv %d\n", this->_measure_events.minor_time.time, this->_measure_events.minor_time.number_of_intervals);
  Serial.printf("Spike time: time %d, no of itv %d\n", this->_measure_events.spike_time.time, this->_measure_events.spike_time.number_of_intervals);
}

int IOT_device::get_node_role()
{
  return this->_device_role;
}

int IOT_device::set_node_role(int role)
{
  this->_device_role = role;
  return 0;
}

void IOT_device::handle_get_node_role()
{
  if (this->_device_flags.current_data_signal == DISPLAY_DATA_SIGNAL)
  {
    if (this->_device_role == NODE_ACTUATOR_DEVICE || this->_device_role == NODE_ACTUATOR_STATION)
    {
      this->_display.send_data_to_display("notify_label.txt=\"Cannot access nodes from node actuator!\"");
    }
  }
  else if (this->_device_flags.current_data_signal == SERIAL_DATA_SIGNAL)
  {
    Serial.printf("DEVICE_ROLE %d\n",this->_device_role);
  }
}

void IOT_device::handle_print_data_bucket(int type)
{
  if (this->_device_role == NODE_GATEWAY || this->_device_role == NODE_USER)
  {
    this->_display.send_data_to_display("page note_page");
    this->_display.send_data_to_display("notify_label.txt=\"This monitor is only for sink node and !\"");
    return;
  }

  if (type == DSP_ENTER)
  {
    if (xSemaphoreTake(_database_mutex, 100) == pdTRUE)
    {
      this->_database.get_number_of_line(SD, SENSOR_RECORD_FILE_PATH, &this->_sensor_file_num_of_lines);
      xSemaphoreGive(_database_mutex);
    }
  }
  else if (type == DSP_UP)
  {
    if (_dsp_current_line_bucket - 5 > 0) _dsp_current_line_bucket -= 5;
  }
  else if (type == DSP_DOWN)
  {
    if (_dsp_current_line_bucket + 5 < this->_sensor_file_num_of_lines) _dsp_current_line_bucket += 5;
  }

  memset(this->_buffer, 0, SERIAL_INPUT_MAX_BUFFER);
  memset(this->_dsp_bucket_buffer, 0, SERIAL_INPUT_MAX_BUFFER);

  // strcat(this->_buffer,"t1.txt=\"");
  if (xSemaphoreTake(_database_mutex, 100) == pdTRUE)
  {
    this->_database.read_line(SD, SENSOR_RECORD_FILE_PATH, this->_dsp_bucket_buffer, _dsp_current_line_bucket);
    xSemaphoreGive(_database_mutex);
  }
  sprintf(this->_buffer,"t1.txt=\"%s\\r\"", this->_dsp_bucket_buffer);
  // strcat(this->_buffer,this->_dsp_bucket_buffer);
  // strcat(this->_buffer,"\\r\"");
  this->_display.send_data_to_display(this->_buffer);
  
  for (int i = 1; i < 5; i++)
  {
    memset(this->_buffer, 0, SERIAL_INPUT_MAX_BUFFER);
    memset(this->_dsp_bucket_buffer, 0, SERIAL_INPUT_MAX_BUFFER);
    // strcat(this->_buffer,"t1.txt+=\"");
    if (xSemaphoreTake(_database_mutex, 100) == pdTRUE)
    {
      this->_database.read_line(SD, SENSOR_RECORD_FILE_PATH, this->_dsp_bucket_buffer, _dsp_current_line_bucket+i);
      xSemaphoreGive(_database_mutex);
    }
    sprintf(this->_buffer,"t1.txt+=\"%s\\r\"", this->_dsp_bucket_buffer);
    // strcat(this->_buffer,this->_dsp_bucket_buffer);
    // strcat(this->_buffer,"\\r\"");
    this->_display.send_data_to_display(this->_buffer);
  }
}

int IOT_device::save_data_to_database()
{
  int i = 0;
  memset(this->_sensor_buffer,0,SENSOR_MAX_BUFFER);  
  sprintf(this->_sensor_buffer,"%d,%d,%.2f,%.2f,%.2f,%d",
          this->_sensors_data_bucket.co2_data, 
          this->_sensors_data_bucket.ch4_data,
          this->_sensors_data_bucket.n2o_data,
          this->_sensors_data_bucket.temp_data,
          this->_sensors_data_bucket.humid_data,
          this->_sensors_data_bucket.pressure_data
          );
    // store to database
  while (i < 6)
  {
    if (xSemaphoreTake(_database_mutex, 100) == pdTRUE)
    {
      if (this->_database.append_file(SD, SENSOR_RECORD_FILE_PATH, this->_sensor_buffer, true) == DATABASE_SUCCESS_WRITE_FILE)
      {
        xSemaphoreGive(_database_mutex);
        break;
      }
      xSemaphoreGive(_database_mutex);
    }
    i++;
    if (i >= 5) return 1; // failed to open file
    vTaskDelay(50);
    // notify error when appending to file failed
  }

  return 0;
}

int block_sending_var = DISABLE;
int signal_timer;
void IOT_device::handle_save_to_database()
{
  int item;
  uint32_t tick;
  if (block_sending_var == DISABLE)
  {
    if (this->_device_flags.data_sensor_remaining_flag == SET)
    {
      Serial.println("DB: sending data sensor...");
      tick = 0;
      memset(this->_sensor_buffer, 0, SENSOR_MAX_BUFFER);
      
      if (xSemaphoreTake(_database_mutex, 100) == pdTRUE)
      {
        this->_database.read_line(SD, SENSOR_RECORD_FILE_PATH, this->_sensor_buffer, this->_sensor_data_remain);
        xSemaphoreGive(_database_mutex);
      }
      // read data sensor and store to db buffer
      sprintf(this->_database_buffer, "SDS,%s",this->_sensor_buffer);
      // notify cmd task to read data
      int msg = DATABASE_DATA_SIGNAL;
      if (xQueueSend(this->_cmd_msg_queue, (void *)&msg, 100) != pdTRUE)
      {
        Serial.println("From Database: CMD event queue is full");
      }

      // start timer to check timeout
      block_sending_var = ENABLE;
      xTimerStart(_db_wait_timeout_timer, portMAX_DELAY);
    }
    else
    {
      Serial.println("DB: wait for event...");
      tick = portMAX_DELAY;
    }
  }
  
  if (xQueueReceive(_store_db_msg_queue, (void *)&item, tick) == pdTRUE)
  {
    if (item == SAVE_DATA_SENSOR_SIGNAL)
    {
      this->save_data_to_database();
      this->send_sensor_data_to_display();
      this->reset_sensor_data();
      this->_sensor_data_len += 1;
      this->_device_flags.data_sensor_remaining_flag = SET;
      block_sending_var = DISABLE;
    }
    else if (item == DATABASE_SEND_DATA_TIMEOUT_SIGNAL)
    {
      // delay for 30 sec
      Serial.println("DB: time out sending data sensor");
      xTimerStart(_db_delay_timer, portMAX_DELAY);
      block_sending_var = ENABLE;
      // resent the current data again
    }
    else if (item == DATABASE_SEND_DATA_SUCCESSFULLY_SIGNAL)
    {
      // increase remain sensor data
      Serial.println("DB: sending data sensor successfully");
      xTimerStop(_db_wait_timeout_timer, portMAX_DELAY);
      block_sending_var = DISABLE;
      this->_device_flags.data_sensor_remaining_flag = SET;
      this->_sensor_data_remain += 1;
      if (xSemaphoreTake(_database_mutex, 100) == pdTRUE)
      {
        this->_database.write_file(SD, DATA_REMAIN_FILE_PATH, String(this->_sensor_data_remain).c_str(),false);
        xSemaphoreGive(_database_mutex);
      }
      if ( this->_sensor_data_remain > this->_sensor_data_len)
      {
        Serial.println("DB: data reach final line");
        this->_device_flags.data_sensor_remaining_flag = RESET;
      }
    }
    else if (item == DELAY_TIMEOUT_SIGNAL)
    {
      Serial.println("DB: time out delay");
      block_sending_var = DISABLE;
    }

  }
  if (db_delay_timeout_flag == 1)
  {
    signal_timer = DATABASE_SEND_DATA_TIMEOUT_SIGNAL;
    xQueueSend(_store_db_msg_queue, (void *)&signal_timer, 0);
    db_delay_timeout_flag = 0;
  }
  else if(db_send_data_timeout_flag == 1)
  {
    signal_timer = DELAY_TIMEOUT_SIGNAL;
    xQueueSend(_store_db_msg_queue, (void *)&signal_timer, 0);
    db_send_data_timeout_flag = 0;
  }
}

void IOT_device::calculate_sensor_data()
{
  int no_of_sample = this->_measure_events.spike_time.number_of_intervals;

  this->_sensors_data_bucket.co2_data      = this->_sensors_data_bucket.co2_data      / no_of_sample;
  this->_sensors_data_bucket.ch4_data      = this->_sensors_data_bucket.ch4_data      / no_of_sample;
  this->_sensors_data_bucket.n2o_data      = this->_sensors_data_bucket.n2o_data      / (float)no_of_sample;
  this->_sensors_data_bucket.temp_data     = this->_sensors_data_bucket.temp_data     / (float)no_of_sample;
  this->_sensors_data_bucket.humid_data    = this->_sensors_data_bucket.humid_data    / (float)no_of_sample;
  this->_sensors_data_bucket.pressure_data = this->_sensors_data_bucket.pressure_data / no_of_sample;
}

void IOT_device::collect_sensor_data()
{
  this->_sensors_data_bucket.co2_data      += this->_sensors.get_data_co2();
  this->_sensors_data_bucket.ch4_data      += this->_sensors.get_data_ch4();
  this->_sensors_data_bucket.n2o_data      += this->_sensors.get_data_no2();
  this->_sensors_data_bucket.temp_data     += this->_sensors.get_data_temp_humid(0); // get temperature
  this->_sensors_data_bucket.humid_data    += this->_sensors.get_data_temp_humid(1); // get humidity
  this->_sensors_data_bucket.pressure_data += this->_sensors.get_data_pressure(); // get humidity
}

void IOT_device::reset_sensor_data()
{
  this->_sensors_data_bucket.co2_data      = 0;
  this->_sensors_data_bucket.ch4_data      = 0;
  this->_sensors_data_bucket.n2o_data      = 0;
  this->_sensors_data_bucket.temp_data     = 0;
  this->_sensors_data_bucket.humid_data    = 0;
  this->_sensors_data_bucket.pressure_data = 0;
}

void IOT_device::handle_set_date_time()
{
  int date_time_arr[] = {0, 0, 0, 1, 1, 2024};
  char time_buffer[5];
  
  for (int i = 0; i < 6; i++)
  {
    memset(time_buffer, 0, 5);
    split_string_char(this->_buffer, ',', i+1, time_buffer);
    date_time_arr[i] = convert_string_to_int(time_buffer, strlen(time_buffer));
  }
  this->_rtc.set_time(date_time_arr[0], date_time_arr[1], date_time_arr[2], date_time_arr[3], date_time_arr[4], date_time_arr[5]);
}

void IOT_device::handle_save_setting()
{
  // save event time
  int len = this->_measure_events.event_time.number_of_event_time;
  Serial.printf("Len: %d",len);
  memset(this->_setting_buffer, 0, SETTING_MAX_BUFFER);
  this->_database.write_file(SD, SETTING_FILE_PATH, "NO_OP", true);

  for (int i = 0; i < len; i++)
  {
    sprintf(this->_setting_buffer,"EVENT_TIME_SET,%d,%d\n",
            this->_measure_events.event_time.time[i].hours, this->_measure_events.event_time.time[i].minutes);

    this->_database.append_file(SD, SETTING_FILE_PATH, this->_setting_buffer, false);
    memset(this->_setting_buffer, 0, strlen(this->_setting_buffer));
  }

  memset(this->_setting_buffer, 0, SETTING_MAX_BUFFER);
  sprintf(this->_setting_buffer,"MAJOR_TIME_SET,%d,%d\n",
            this->_measure_events.major_time.number_of_intervals, this->_measure_events.major_time.time);

    Serial.println(this->_setting_buffer);
  this->_database.append_file(SD, SETTING_FILE_PATH, this->_setting_buffer,false);

  memset(this->_setting_buffer, 0, SETTING_MAX_BUFFER);
  sprintf(this->_setting_buffer,"MINOR_TIME_SET,%d,%d\n",
            this->_measure_events.minor_time.number_of_intervals, this->_measure_events.minor_time.time);

    Serial.println(this->_setting_buffer);
  this->_database.append_file(SD, SETTING_FILE_PATH, this->_setting_buffer,false);

  memset(this->_setting_buffer, 0, SETTING_MAX_BUFFER);
  sprintf(this->_setting_buffer,"SPIKE_TIME_SET,%d,%d\n",
            this->_measure_events.spike_time.number_of_intervals, this->_measure_events.spike_time.time);

    Serial.println(this->_setting_buffer);
  this->_database.append_file(SD, SETTING_FILE_PATH, this->_setting_buffer,false);
}

int IOT_device::on()
{
  int time_check;
  // Serial.println("PASS ON");
  switch (this->_device_on_state)
  {
    // Serial.printf("state: %d\n",this->_device_on_state);
    case DEVICE_ON_IDLE:
    {
      // Serial.printf("state: %d\n",this->_device_on_state);
      if (this->check_is_measure_event_start())
      {
        this->_device_on_state = DEVICE_ON_START_MEASURE_EVENT;
      }
      else if (this->_device_flags.forcing_to_event_flag == ENABLE)
      {
        this->_device_flags.forcing_to_event_flag = DISABLE;  // disable this flag to run only one time
        this->_device_on_state = DEVICE_ON_START_MEASURE_EVENT;
      }
      Serial.println("On idle, waiting for measure event...");
      vTaskDelay(1000);
      break;
    }
    case DEVICE_ON_START_MEASURE_EVENT:
    {
      this->timer_start();
      // close the chamber and check if timeout occured
      // check sensor is already
      this->timer_stop();
      this->timer_reset();
      this->reset_timer_counter();
      this->timer_start();

      this->_device_flags.forcing_to_event_flag = ENABLE;
      this->_device_flags.major_time_on_flag    = ENABLE;
      this->_device_flags.minor_time_on_flag    = ENABLE;
      this->_device_flags.spike_time_on_flag    = ENABLE;
      
      Serial.println("Starting measure event");
      this->_setpoint_chamber_door_time.time = this->get_timer_counter();
      this->_device_on_state = DEVICE_ON_MAJOR_TIME;
      break;
    }
    case DEVICE_ON_END_MEASURE_EVENT:
    {
      this->reset_measure_event_params();
      this->_device_on_state = DEVICE_ON_IDLE;
      break;
    }
    case DEVICE_ON_ERROR:
    {
      Serial.println("Device get an operation error!");
      vTaskDelay(100);
      break;
    }
    case DEVICE_ON_WAITING_DOOR:
    {
      time_check = this->get_timer_counter();

      if (time_check - this->_setpoint_chamber_door_time.time > this->_timeout_chamber_door)
      {
        this->_device_on_state = DEVICE_ON_ERROR;
      }
      // if ()
      // {
      //   this->_device_on_state = DEVICE_ON_MAJOR_TIME;
      // }
      // check whether chamber door is closed or not, if it is, jump to major time state, else jump to error state
      break;
    }
    case DEVICE_ON_MAJOR_TIME:
    {
      Serial.println("--- Step into major state ---");
      if (this->_setpoint_measure_major_time.number_of_intervals < this->_measure_events.major_time.number_of_intervals)
      {
        time_check = this->get_timer_counter();
        
        if ((time_check / 60) - this->_setpoint_measure_major_time.time >= this->_measure_events.major_time.time)
        {
          this->_device_flags.major_time_on_flag = ENABLE;
        }

        if (this->_device_flags.major_time_on_flag == ENABLE)
        {
          Serial.println("  Major: time on");
          Serial.printf("   Number of major counter: %d\n", this->_setpoint_measure_major_time.number_of_intervals);
          this->_setpoint_measure_major_time.time                 = (this->get_timer_counter() / 60);
          this->_setpoint_measure_major_time.number_of_intervals  += 1;
          this->_setpoint_measure_minor_time.number_of_intervals  = 0;
          this->_device_flags.major_time_on_flag                  = DISABLE;
          this->_actuators.set_actuators_pin(FAN_1_PIN, HIGH);
          // close chamber
          this->_actuators.set_actuators_pin(CYLINDER_1_PIN, HIGH);
          this->_actuators.set_actuators_pin(CYLINDER_2_PIN, LOW);
          this->set_chamber_door(ON);
          // this->_actuators.set_actuators_pin(FAN_2_PIN, HIGH);
          this->_device_on_state                                  = DEVICE_ON_MINOR_TIME;
        }
        else
        {
          vTaskDelay(1000);
        }
      }
      else
      {
        this->_device_on_state = DEVICE_ON_END_MEASURE_EVENT;
      }
      break;
    }
    case DEVICE_ON_MINOR_TIME:
    {
      Serial.println("--- Step into minor state ---");
      // time_check = this->get_timer_counter();
      
      if (this->_setpoint_measure_minor_time.number_of_intervals < this->_measure_events.minor_time.number_of_intervals)
      {
        this->_actuators.set_actuators_pin(CYLINDER_1_PIN, HIGH);
        this->_actuators.set_actuators_pin(CYLINDER_2_PIN, LOW);
        time_check = this->get_timer_counter();
       
        if ((time_check/60) - this->_setpoint_measure_minor_time.time >= this->_measure_events.minor_time.time)
        {
          this->_device_flags.minor_time_on_flag = ENABLE;
        }

        if (this->_device_flags.minor_time_on_flag == ENABLE)
        {
          Serial.println("  Minor: time on");
          Serial.printf("   Number of minor counter: %d\n", this->_setpoint_measure_minor_time.number_of_intervals);
          this->_setpoint_measure_minor_time.time = this->get_timer_counter() / 60;
          this->_setpoint_measure_minor_time.number_of_intervals += 1;
          // reset the counter each time the setpoint change state
          this->_setpoint_measure_spike_time.number_of_intervals = 0; 
          this->_device_flags.minor_time_on_flag = DISABLE;
          this->_device_on_state = DEVICE_ON_SPIKE_TIME;
        }
        else
        {
          vTaskDelay(1000);
        }
      }
      else
      {
        // open the chamber door
        // keep the fan 1 and 2 operate in ? minutes
        
        this->_setpoint_fan_still_time.time = this->get_timer_counter();
        this->set_chamber_door(OFF);
        this->_device_on_state = DEVICE_ON_FAN_STILL_TIME;
      }
      break;
    }
    case DEVICE_ON_SPIKE_TIME:
    {
      // Serial.println("--- Step into spike state ---");
      // this->_actuators.set_actuators_pin(CYLINDER_1_PIN, LOW);
      // this->_actuators.set_actuators_pin(CYLINDER_2_PIN, HIGH);
      
      if (this->_setpoint_measure_spike_time.number_of_intervals < this->_measure_events.spike_time.number_of_intervals)
      {
        // get current time in seconds
        time_check = this->get_timer_counter();

        // if time interval reached
        if (time_check - this->_setpoint_measure_spike_time.time >= this->_measure_events.spike_time.time)
        {
          this->_device_flags.spike_time_on_flag = ENABLE;
        }

        if (this->_device_flags.spike_time_on_flag == ENABLE)
        {
          Serial.println("  Spike: time on");
          Serial.printf("   Number of spike counter: %d\n", this->_setpoint_measure_spike_time.number_of_intervals);
          // Reset the spike time point
          this->_setpoint_measure_spike_time.time = this->get_timer_counter();
          // increase number of time point
          this->_setpoint_measure_spike_time.number_of_intervals += 1;
          this->_device_flags.spike_time_on_flag = DISABLE;
          this->_device_on_state = DEVICE_ON_GET_SENSOR_VALUE;
        }
        else
        {
          vTaskDelay(100);
          // Serial.println("Spike: time on");
        }
      }
      else
      {
        int num = SAVE_DATA_SENSOR_SIGNAL;
        this->calculate_sensor_data();

        if (xQueueSend(_store_db_msg_queue, (void *)&num, 100) != pdTRUE)
        {
          Serial.println("Store to db event queue is full");
        }
        
        this->_device_on_state = DEVICE_ON_MINOR_TIME;
      }
      break;
    }
    case DEVICE_ON_GET_SENSOR_VALUE:
    {
      time_check = this->get_timer_counter();
      this->callbackfnc();
      this->_sensors.read();
      this->collect_sensor_data();
      this->_sensors.print_data_serial();
      Serial.println("Get sensor value...");
      this->_device_on_state = DEVICE_ON_SPIKE_TIME;
      break;
    }
    case DEVICE_ON_FAN_STILL_TIME:
    {
      Serial.println("Fan's still on until off...");
      time_check = this->get_timer_counter();

      if ((time_check - this->_setpoint_fan_still_time.time) > this->_measure_events.fan_still_time.time)
      {
        this->_actuators.set_actuators_pin(FAN_1_PIN, LOW);
        this->_actuators.set_actuators_pin(CYLINDER_1_PIN, LOW);
        this->_actuators.set_actuators_pin(CYLINDER_2_PIN, HIGH);
        // this->_actuators.set_actuators_pin(FAN_2_PIN, LOW);
        this->_device_on_state = DEVICE_ON_MAJOR_TIME;
      }
      vTaskDelay(1000);
      break;
    }
    default:
    {
      this->_device_on_state = DEVICE_ON_IDLE;
    }
  }

  return 0;
}

void IOT_device::off()
{
  this->reset_measure_event_params();
  return;
}

void IOT_device::sample()
{
    // Serial.printf("Timer count on: %d", Timer_counter);
  Serial.println("----- Sample mode -----");
  this->_sensors.read();
  this->_sensors.print_data_serial();
  this->_rtc.print_get_data_time("%A, %B %d %Y %H:%M:%S");
  vTaskDelay(1000/portTICK_PERIOD_MS);
}

void IOT_device::set_device_state(int state)
{
  if (state == 0)
  {
    _device_state = DEVICE_STATE_ON;
    return;
  }
  else if (state == 1)
  {
    _device_state = DEVICE_STATE_OFF;
    return;
  }
  // Serial.println("PASS");
  _device_state = DEVICE_STATE_SAMPLE;
}

int IOT_device::get_device_state()
{
  return this->_device_state;
}

int IOT_device::get_device_on_state()
{
  return this->_device_on_state;
}

/*
 * handle command from serial
 */

void IOT_device::handle_command_serial()
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

uint8_t IOT_device::handle_load_setting_from_db()
{
  int number_of_line = 0;
  String event_setting_cmd = "";
  // this code is for creating setting file if the file is missing
  File file = SD.open(SETTING_FILE_PATH);
  if(!file) 
  {
    Serial.println("Failed to open event setting file, creating new one");
    file = SD.open(SETTING_FILE_PATH, FILE_WRITE);
    file.close();
    this->init_measure_event_time();                      // default setting
    return 0;
  }
  file.close();
  // end code

  this->_database.get_number_of_line(SD, SETTING_FILE_PATH, &number_of_line);
  Serial.printf("Number of line event setting: %d\n",number_of_line);
  for (int i = 0; i < number_of_line; i++)
  {
    // bool read_line(fs::FS &fs, const char * path, String &lineStorage, int lineNumber);
    this->_database.read_line(SD, SETTING_FILE_PATH, event_setting_cmd, i+1);
    memset(this->_buffer, 0, strlen(this->_buffer));
    strcpy(this->_buffer, event_setting_cmd.c_str());
    Serial.println(this->_buffer);
    this->process_cmd();
  }
  return 0;
}

void IOT_device::handle_get_data_from_display()
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

void IOT_device::handle_data_from_transceiver()
{
  if (Serial2.available())
  {
    vTaskDelay(20);
    char c;
    int i = 0;
    memset(this->_LoRa_node_buffer, 0, SETTING_MAX_BUFFER);
    // String command = "";
    while (Serial2.available())
    {
      c = Serial2.read();
      // Serial.print(c);
      if (c == '\n') break;
      this->_LoRa_node_buffer[i] = c;
      i++;
    }

    int msg = LORA_NODE_DATA_SIGNAL;
    if (xQueueSend(this->_cmd_msg_queue, (void *)&msg, 100) != pdTRUE)
    {
      Serial.println("From LoRa serial: CMD event queue is full");
    }
    // Serial.println();
    // Serial.println(this->_buffer);
    // handle_cmd();
  }

  int msg;
  if (xQueueReceive(this->_LoRa_msg_queue, (void *)&msg, 0) == pdTRUE)
  {
    if (msg == DATABASE_DATA_SIGNAL)
    {
      Serial2.println(this->_database_buffer);
    }
  }
}

void IOT_device::display_update_device_state()
{
  if (this->_device_state == DEVICE_STATE_ON)
  {
    this->_display.send_data_to_display("t1.txt=\"ON\"");
  }
  else
  {
    this->_display.send_data_to_display("t1.txt=\"OFF\"");
  }
}

void IOT_device::display_update_event_time()
{
  int j = 0, k = 4;
  char temp[3];
  split_string_char(this->_buffer, ',', 1, temp);
  int n = convert_string_to_int(temp,2);
  
  for (int i = n-1; i < n+3; i++)
  {
    memset(Event_time_string_temp, 0, 20);
    sprintf(Event_time_string_temp,"b%d.txt=\"%d:%d\"",k,
                      this->_measure_events.event_time.time[i].hours,
                      this->_measure_events.event_time.time[i].minutes);
    this->_display.send_data_to_display(Event_time_string_temp);
    j += 1;
    k += 1;
  }
  // Event_time_string_temp[6];
}

void IOT_device::display_update_measure_time(Measure_time_t *m_time, int time_type)
{
  int j;
  char temp[4];
  if (time_type == MAJOR_TIME) j = 0;
  else if (time_type == MINOR_TIME) j = 2;
  else if (time_type == SPIKE_TIME) j = 4;

  memset(Event_time_string_temp, 0, 20);
  strcat(Event_time_string_temp, Dsp_MT_current_label[j]);

  memset(temp, 0, 4);
  convert_int_to_str(m_time->time,temp);
  strcat(Event_time_string_temp, temp);
  this->_display.send_data_to_display(Event_time_string_temp);
  j += 1;
  
  memset(Event_time_string_temp, 0, 20);
  strcat(Event_time_string_temp, Dsp_MT_current_label[j]);

  memset(temp, 0, 3);
  convert_int_to_str(m_time->number_of_intervals,temp);
  strcat(Event_time_string_temp, temp);
  this->_display.send_data_to_display(Event_time_string_temp);
}

void IOT_device::display_print_date_time()
{
  char temp[3];

  memset(Event_time_string_temp, 0, 20);
  sprintf(Event_time_string_temp, "t14.txt=\"%d:%d\"",this->_rtc.get_hours(), this->_rtc.get_minutes());

  this->_display.send_data_to_display(Event_time_string_temp);
}

int IOT_device::handle_cmd()
{
  // Serial.println("Pass here?");
  int item;
  if (xQueueReceive(this->_cmd_msg_queue, (void *)&item, portMAX_DELAY) == pdTRUE)
  {
    if (item == SERIAL_DATA_SIGNAL)
    {
      Serial.println("From serial");
      this->_device_flags.current_data_signal = SERIAL_DATA_SIGNAL;
      strcpy(this->_buffer, this->_serial_buffer);
    }
    else if (item == BUTTONS_DATA_SIGNAL)
    {
      Serial.println("From buttons");
      this->_device_flags.current_data_signal = BUTTONS_DATA_SIGNAL;
      strcpy(this->_buffer, this->_buttons_buffer);
    }
    else if (item == DISPLAY_DATA_SIGNAL)
    {
      Serial.println("From display");
      this->_device_flags.current_data_signal = DISPLAY_DATA_SIGNAL;
      strcpy(this->_buffer, this->_display_buffer);
    }
    else if (item == LORA_NODE_DATA_SIGNAL)
    {
      Serial.println("From LoRa");
      this->_device_flags.current_data_signal = LORA_NODE_DATA_SIGNAL;
      strcpy(this->_buffer, this->_LoRa_node_buffer);
    }
    else if (item == DATABASE_DATA_SIGNAL)
    {
      Serial.println("From Database");
      this->_device_flags.current_data_signal = DATABASE_DATA_SIGNAL;
      strcpy(this->_buffer, this->_database_buffer);
    }
    this->process_cmd();
    // Serial.println("PASS");
  }
  return 0;
}

int IOT_device::process_cmd()
{
  if (str_startswith(this->_buffer, "DEVICE_ON"))
  {
    // Serial.println("PASS");
    this->set_device_state(0);
    if (this->_device_flags.current_data_signal == DISPLAY_DATA_SIGNAL){}
      this->display_update_device_state();
  }
  else if (str_startswith(this->_buffer, "DEVICE_OFF"))
  {
    this->set_device_state(1);
    if (this->_device_flags.current_data_signal == DISPLAY_DATA_SIGNAL){}
      this->display_update_device_state();
  }
  else if (str_startswith(this->_buffer, "DEVICE_SAMPLE"))  { this->set_device_state(2); }
  else if (str_startswith(this->_buffer, "FORCE_EVENT_ON"))  { this->_device_flags.forcing_to_event_flag = ENABLE; }
  else if (str_startswith(this->_buffer, "F12_ON"))          { this->_actuators.set_actuators_pin(FAN_1_PIN, HIGH); }
  else if (str_startswith(this->_buffer, "F12_OFF"))         { this->_actuators.set_actuators_pin(FAN_1_PIN, LOW); }
  else if (str_startswith(this->_buffer, "CYLINDER_ON"))          { 
    this->_actuators.set_actuators_pin(CYLINDER_1_PIN, HIGH); 
    this->_actuators.set_actuators_pin(CYLINDER_2_PIN, LOW); 
  }
  else if (str_startswith(this->_buffer, "CYLINDER_OFF"))         { 
    this->_actuators.set_actuators_pin(CYLINDER_1_PIN, LOW); 
    this->_actuators.set_actuators_pin(CYLINDER_2_PIN, HIGH); 
  }
  else if (str_startswith(this->_buffer, "SENSORS_READ"))
  {
    this->_sensors.read();
    this->_sensors.print_data_serial();
  } 
  // else if (str_startswith(this->_buffer, "CYLINDER2_ON"))          { this->_actuators.set_actuators_pin(PUMP_1_PIN, HIGH); }
  // else if (str_startswith(this->_buffer, "CYLINDER2_OFF"))         { this->_actuators.set_actuators_pin(PUMP_1_PIN, LOW); }
  else if (str_startswith(this->_buffer, "SDS")) // SDS,(data sensor)
  {
    if (strlen(this->_buffer) > 74)
    {
      Serial.println("Data send to sink node is too long! Ignore");
      int msg = DATABASE_SEND_DATA_SUCCESSFULLY_SIGNAL;
      if (xQueueSend(this->_store_db_msg_queue, (void *)&msg, 100) != pdTRUE)
      {
        Serial.println("From cmd : db event queue is full");
      }
    }
    else
    {
      this->_buffer[strlen(this->_buffer)-1] = 0;
      Serial.println("Data sending to user node via serial port ...");
      Serial2.println(this->_buffer);
    }
    // this->handle_get_measure_event_time(EVENT_TIME_ADD);
  }
  else if (str_startswith(this->_buffer, "MSG00")) // SEND_DTSS,(data)
  {
    Serial.println(this->_buffer);
    int msg = DATABASE_SEND_DATA_SUCCESSFULLY_SIGNAL;
    if (xQueueSend(this->_store_db_msg_queue, (void *)&msg, 100) != pdTRUE)
    {
      Serial.println("From cmd : db event queue is full");
    }
    // this->handle_get_measure_event_time(EVENT_TIME_ADD);
  }
  else if (str_startswith(this->_buffer, "EVENT_TIME_SET")) // EVENT_TIME_SET,hours,minutes
  {
    this->handle_get_measure_event_time(EVENT_TIME_ADD);
  }
  else if (str_startswith(this->_buffer, "ET_UPDATE")) // ET_UPDATE,position,hours,minutes
  {
    this->handle_get_measure_event_time(EVENT_TIME_UPDATE);
  }
  else if (str_startswith(this->_buffer, "ET_DELETE")) // EVENT_DELETE,position
  {
    this->handle_get_measure_event_time(EVENT_TIME_DELETE);
  }
  else if (str_startswith(this->_buffer, "ET_DEL_TAIL")) // EVENT_DEL_TAIL
  {
    this->handle_get_measure_event_time(EVENT_TIME_DELETE_TAIL);
  }
  else if (str_startswith(this->_buffer, "MAJOR_TIME_SET")) // MAJOR_TIME_SET,no_of_interval,minutes
  {
    this->handle_get_measure_event_time(MAJOR_TIME);
  }
  else if (str_startswith(this->_buffer, "MINOR_TIME_SET")) // MINOR_TIME_SET,no_of_interval,minutes
  {
    this->handle_get_measure_event_time(MINOR_TIME);
  }
  else if (str_startswith(this->_buffer, "SPIKE_TIME_SET")) // SPIKE_TIME_SET,no_of_interval,seconds
  {
    this->handle_get_measure_event_time(SPIKE_TIME);
  }
  else if (str_startswith(this->_buffer, "PRINT_MEASURE_TIME_EVENT")) // PRINT_MEASURE_TIME_EVENT
  {
    this->print_measure_time_event();
  }
  else if (str_startswith(this->_buffer, "SAVE_ET_SETTING")) // SHOW_DATE 
  {
    this->handle_save_setting();
  }
  else if (str_startswith(this->_buffer, "SET_RTC_TIME")) // SET_RTC_TIME,hours,minutes,seconds,day,month,year 
  {
    this->handle_set_date_time();
  }
  else if (str_startswith(this->_buffer, "SHOW_DATE_TIME")) // SHOW_DATE 
  {
    this->_rtc.print_get_data_time();
  }
  else if (str_startswith(this->_buffer, "GET_GPS")) //GET_GPS
  {
    this->_gps.display_info(true, false, false);
  }
  else if (str_startswith(this->_buffer, "LIST_DIR")) // LIST_DIR
  { 
    this->_database.list_directory(SD, "/", 3);
  }
  else if (str_startswith(this->_buffer, "GET_NODE_ROLE")) // GET_NODE_ROLE
  { 
    this->handle_get_node_role();
  }
  else if (str_startswith(this->_buffer, "DSP_SYS_STATE")) // for Display
  {
    this->display_update_device_state();
  }
  else if (str_startswith(this->_buffer, "DSP_ET_UPDATE")) // for Display
  {
    this->display_update_event_time();
  }
  else if (str_startswith(this->_buffer, "DSP_GET_DT")) // for Display
  {
    this->display_print_date_time();
  }
  else if (str_startswith(this->_buffer, "DSP_MT_UPDATE")) // for Display
  {
    this->display_update_measure_time(&this->_measure_events.major_time, MAJOR_TIME);
    this->display_update_measure_time(&this->_measure_events.minor_time, MINOR_TIME);
    this->display_update_measure_time(&this->_measure_events.spike_time, SPIKE_TIME);
  }
  else if (str_startswith(this->_buffer, "DSP_BP_E")) // for Display
  {
    this->handle_print_data_bucket(DSP_ENTER);
  }
  else if (str_startswith(this->_buffer, "DSP_BP_H")) // for Display
  {
    this->handle_print_data_bucket(DSP_UP);
  }
  else if (str_startswith(this->_buffer, "DSP_BP_D")) // for Display
  {
    this->handle_print_data_bucket(DSP_DOWN);
  }
  else
  {
    Serial.println("Invalid command! Check is correct syntax");
  }
  Serial.println("Command processed!");
  return 0;
}

void IOT_device::handle_buttons_input()
{
  if (this->_buttons.read(BUTTON_4, NULL) == ON)
  {

  }
  else if (this->_buttons.read(BUTTON_2, NULL) == ON)
  {
    this->_device_flags.current_page_flag = (this->_device_flags.current_page_flag+1) % PAGE_MAX_NUMBER;
    this->_display.print_page_on_display(this->_device_flags.current_page_flag);

    on_buzzer();
  }
  else if (this->_buttons.read(BUTTON_3, NULL) == ON)
  {
    if (this->get_device_state() == DEVICE_STATE_OFF || this->get_device_state() == DEVICE_STATE_FLOAT)
    {
      this->set_device_state(DEVICE_STATE_ON);
    }
    else
    {
      this->set_device_state(DEVICE_STATE_OFF);
    }
    on_buzzer();
  }
  else if (this->_buttons.read(BUTTON_1, NULL) == ON)
  {
    if (this->get_device_state() == DEVICE_STATE_ON)
    {
      this->_device_flags.forcing_to_event_flag = ENABLE;
    }

    on_buzzer();
  }
}

int IOT_device::timer_setup(int timer = 0, int prescaler = 80, bool counter = true, void (*on_timer)() = NULL)
{
  this->_my_timer = timerBegin(timer, prescaler, counter);
  timerAttachInterrupt(this->_my_timer, on_timer, true);
  timerAlarmWrite(this->_my_timer, 1000000, true);
  // timerAlarmEnable(this->_my_timer);
  return 0;
}

int IOT_device::get_timer_counter()
{
  return Timer_counter;
}

void IOT_device::reset_timer_counter()
{
  Timer_counter = 0;
}

void IOT_device::timer_start()
{
  timerAlarmEnable(this->_my_timer);
}

void IOT_device::timer_stop()
{
  timerAlarmDisable(this->_my_timer);
}

void IOT_device::timer_reset()
{
  timerWrite(this->_my_timer, 0);
}
