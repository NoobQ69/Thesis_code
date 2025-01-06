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
#ifndef IOT_MONITORING_SYSTEM_DATABASE_H_
#define IOT_MONITORING_SYSTEM_DATABASE_H_

#include "FS.h"
#include "SPI.h"
#include "SD.h"
#include "database.h"
#include "utilities.h"

#define SD_CS_PIN        5
#define SD_CLK_PIN       18
#define SD_MOSI_PIN      23
#define SD_MISO_PIN      19

class IOT_database
{
  private:
    
  public:
    IOT_database();
    int  begin();
    int  list_directory(fs::FS &fs, const char * dirname, uint8_t levels);
    int  create_diretory(fs::FS &fs, const char * path);
    int  remove_directory(fs::FS &fs, const char * path);
    void get_SD_card_info();
    int  read_file(fs::FS &fs, const char * path, String &text_str, int &number_of_line);
    int  read_file(fs::FS &fs, const char * path, char *text_str, int *line_total);
    int  print_file_to_serial(fs::FS &fs, const char * path);
    int  write_file(fs::FS &fs, const char * path, const char * message, bool is_new_line);
    int  append_file(fs::FS &fs, const char * path, const char * message, bool is_new_line);
    int  rename_file(fs::FS &fs, const char * path1, const char * path2);
    bool delete_file(fs::FS &fs, const char * path);
    bool delete_line(fs::FS &fs, const char * path, int line);
    int get_number_of_line(fs::FS &fs, const char * path, int *line);
    bool read_line(fs::FS &fs, const char * path, char *lineStorage, int lineNumber);
    bool read_line(fs::FS &fs, const char * path, String &lineStorage, int lineNumber);
};

#endif