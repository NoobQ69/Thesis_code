#include "utilities.h"
#include "display.h"
#include <Arduino.h>
#include <string>
#include "SoftwareSerial.h"

SoftwareSerial Display_serial(DSP_RX_PIN, DSP_TX_PIN);

IOT_display::IOT_display()
{
  
}

int IOT_display::begin()
{
  Display_serial.begin(9600);
  this->send_data_to_display("page loading_page");
  return 0;
}

void IOT_display::send_data_to_display(const char *data)
{
  Serial.println(data);
  Display_serial.print(data);
  // Serial2.print("co2_label.txt=" + String(i));
  Display_serial.write(0xff);
  Display_serial.write(0xff);
  Display_serial.write(0xff);
}

int IOT_display::get_data_from_display(char *buffer)
{
    // Serial.println("display task run...");
  int i = 0;
  if (Display_serial.available() > 0)
  {
    vTaskDelay(50);
    char c;
    memset(buffer, 0, SETTING_MAX_BUFFER);
    while (Display_serial.available() > 0)
    {
      // stored into task buffer
      // notify to handle cmd task
      // set busy flag to stop receive data
      // reset busy flag when handle task cmd has read data on the buffer 
      c = Display_serial.read();
      buffer[i] = c;
      i++;
    }
    Serial.printf("Data from display: %s", buffer);
    // handle command from lcd
  }
  return i;
}

void IOT_display::print_page_on_display(int page_num)
{
  if (page_num == -1)
  {
    this->send_data_to_display("page loading_page");
  }
  else if (page_num == 0)
  {
    this->send_data_to_display("page menu_page");
  }
  else if (page_num == 1)
  {
    this->send_data_to_display("page monitor_page");
  }
  else if (page_num == 2)
  {
    this->send_data_to_display("page control_page");
  }
  else if (page_num == 3)
  {
    this->send_data_to_display("page manual_page");
  }
  else if (page_num == 4)
  {
    this->send_data_to_display("page bucket_page");
  }
  else if (page_num == 5)
  {
    this->send_data_to_display("page note_page");
  }
  else if (page_num == 6)
  {
    this->send_data_to_display("page about_page");
  }
  else
  {
    this->send_data_to_display("page menu_page");
  }
}
