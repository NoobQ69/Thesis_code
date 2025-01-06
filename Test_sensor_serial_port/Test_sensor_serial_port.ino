#include "sensors.h"
#include "utilities.h"
IOT_sensors ss1;
TaskHandle_t btn_task_handler       = NULL;
static const BaseType_t App_cpu     = 0;

#define BUTTON_1    39
#define BUTTON_2    36
#define BUTTON_3    35
#define BUTTON_4    34

#define BUZZER_PIN 2

static void on_buzzer(int delay_ms = 200)
{
  digitalWrite(BUZZER_PIN, HIGH);
  vTaskDelay(delay_ms/portTICK_PERIOD_MS);
  digitalWrite(BUZZER_PIN, LOW);
}

int read(int button, void (*callback_func)() = NULL)
{
  Serial.print(digitalRead(BUTTON_1));
  Serial.print(digitalRead(BUTTON_2));
  Serial.print(digitalRead(BUTTON_3));
  Serial.println(digitalRead(BUTTON_4));
  if (digitalRead(button) == LOW)
  {
    while (digitalRead(button) == LOW)
    {
      vTaskDelay(20);
    }

    if (callback_func != NULL)
    {
      callback_func();  // don't put delay to callback function!
    }
    return ON;
  }
  return OFF;
}

void handle_buttons_input()
{
  if (read(BUTTON_4, NULL) == ON)
  {
    Serial.println("Button 4 is pushed");
    on_buzzer();
  }
  else if (read(BUTTON_2, NULL) == ON)
  {
    // this->_device_flags.current_page_flag = (this->_device_flags.current_page_flag+1) % PAGE_MAX_NUMBER;
    // this->_display.print_page_on_display(this->_device_flags.current_page_flag);
    Serial.println("Button 2 is pushed");
    on_buzzer();
  }
  else if (read(BUTTON_3, NULL) == ON)
  {
    // if (this->get_device_state() == DEVICE_STATE_OFF || this->get_device_state() == DEVICE_STATE_FLOAT)
    // {
    //   this->set_device_state(DEVICE_STATE_ON);
    // }
    // else
    // {
    //   this->set_device_state(DEVICE_STATE_OFF);
    // }
    Serial.println("Button 3 is pushed");
    on_buzzer();
  }
  else if (read(BUTTON_1, NULL) == ON)
  {
    // if (this->get_device_state() == DEVICE_STATE_ON)
    // {
    //   this->_device_flags.forcing_to_event_flag = ENABLE;
    // }
    Serial.println("Button 1 is pushed");
    on_buzzer();
  }
}

void buttons_task(void *param)
{
  while (1)
  {
    handle_buttons_input();
    vTaskDelay(100);
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(BUTTON_1, INPUT);
  pinMode(BUTTON_2, INPUT);
  pinMode(BUTTON_3, INPUT);
  pinMode(BUTTON_4, INPUT);
  ss1.begin();

  xTaskCreatePinnedToCore(buttons_task,"Btn task", 4096, NULL,  4, &btn_task_handler, App_cpu);
}

void loop() {
  // put your main code here, to run repeatedly:
  ss1.read();
  vTaskDelay(2000);
}
