#include "database.h"

SPIClass sd_SPI(VSPI);

IOT_database::IOT_database()
{
  // sd_SPI = SPIClass(VSPI);
  // Serial.println("Done 1");
}

int IOT_database::begin()
{
  pinMode(SD_CS_PIN, OUTPUT);  // SS
  sd_SPI.begin(SD_CLK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN); // Initiate SPI bus

  // Serial.println("Done 2");
  if (SD.begin(SD_CS_PIN, sd_SPI))
  {
    this->get_SD_card_info();
    return DATABASE_SUCCESS_INIT;
  }
  // else
  Serial.println("SD card mount failed");
  return DATABASE_ERROR_INIT;
}

int IOT_database::list_directory(fs::FS &fs, const char * dirname, uint8_t levels)
{
  Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  
  if(!root)
  {
    Serial.println("Failed to open directory");
    return DATABASE_ERROR_OPEN_DIRECTORY;
  }
  
  if(!root.isDirectory())
  {
    Serial.println("Not a directory");
    return DATABASE_ERROR_NOT_A_DIRECTORY;
  }

  File file = root.openNextFile();
  
  while(file)
  {
    if(file.isDirectory())
    {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if(levels){
        list_directory(fs, file.name(), levels -1);
      }
    } 
    else
    {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }

  file.close();
  return DATABASE_SUCCESS_OPEN_DIRECTORY;
}

int IOT_database::create_diretory(fs::FS &fs, const char * path)
{
  Serial.printf("Creating Dir: %s\n", path);

  if(fs.mkdir(path))
  {
    Serial.println("Dir created");
    return DATABASE_SUCCESS_CREATE_DIRECTORY;
  }

  Serial.println("mkdir failed");
  return DATABASE_ERROR_CREATE_DIRECTORY;
}

int IOT_database::remove_directory(fs::FS &fs, const char * path)
{
  Serial.printf("Removing Dir: %s\n", path);

  if(fs.rmdir(path))
  {
    Serial.println("Dir removed");
    return DATABASE_SUCCESS_REMOVE_DIRECTORY;
  }

  Serial.println("rmdir failed");
  return DATABASE_ERROR_REMOVE_DIRECTORY;
}

void IOT_database::get_SD_card_info()
{
  uint8_t cardType = SD.cardType();
  Serial.print("SD Card Type: ");

  if (cardType == CARD_MMC)
  {
    Serial.println("MMC");
  } 
  else if (cardType == CARD_SD)
  {
    Serial.println("SDSC");
  } 
  else if (cardType == CARD_SDHC)
  {
    Serial.println("SDHC");
  } 
  else {
    Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
}

int IOT_database::read_file(fs::FS &fs, const char * path, String &text_str, int &number_of_line)
{
  Serial.printf("Reading file: %s\n", path);
  number_of_line = 0;
  File file = fs.open(path);
  
  if(!file) 
  {
    Serial.println("Failed to open file for reading");
    return DATABASE_ERROR_FAILED_OPEN_FILE;
  }
  
  text_str = "";

  while(file.available()) 
  {
    char c = file.read();
    text_str += c;
    if (c == '\n') {
      number_of_line++;
      Serial.println();
    }
    else 
    {
      Serial.print(c);
    }
  }
  
  file.close();
  return DATABASE_SUCCESS_READ_FILE;
}

int IOT_database::read_file(fs::FS &fs, const char * path, char *text_str, int *line_total)
{
  Serial.printf("Reading file: %s\n", path);
  File file = fs.open(path);

  if(!file) 
  {
    Serial.println("Failed to open file for reading");
    return DATABASE_ERROR_FAILED_OPEN_FILE;
  }

  int i = 0;
  char c;

  while(file.available()) 
  {
    c = file.read();
    text_str[i] = c;
    
    if (c == '\n') 
    {
      line_total += 1;
    }
  }
  
  file.close();

  return DATABASE_SUCCESS_READ_FILE;
}

int IOT_database::write_file(fs::FS &fs, const char * path, const char * message, bool is_new_line=false)
{
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if(!file) {
    Serial.println("Failed to open file for writing");
    return DATABASE_ERROR_FAILED_OPEN_FILE;
  }
  
  int result;

  if (is_new_line) 
  {
    result = file.println(message);
  }
  else
  {
    result = file.print(message);
  }
  
  if(!result)
  {
    Serial.println("Write failed");
    file.close();
    return DATABASE_ERROR_FAILED_WRITE_FILE;
  }

  Serial.println("Message appended");
  file.close();

  return DATABASE_SUCCESS_WRITE_FILE;
}

int IOT_database::append_file(fs::FS &fs, const char * path, const char * message, bool is_new_line=false)
{
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if(!file)
  {
    Serial.println("Failed to open file for appending");
    return DATABASE_ERROR_FAILED_OPEN_FILE;
  }

  int result;

  if (is_new_line) 
  {
    result = file.println(message);
  }
  else 
  {
    result = file.print(message);
  }
  
  if(!result)
  {
    Serial.println("Append failed");
    file.close();
    return DATABASE_ERROR_FAILED_WRITE_FILE;
  }
  Serial.println("Message appended");
  file.close();

  return DATABASE_SUCCESS_WRITE_FILE;
}

int IOT_database::rename_file(fs::FS &fs, const char * path1, const char * path2)
{
  Serial.printf("Renaming file %s to %s\n", path1, path2);

  if (fs.rename(path1, path2))
  {
    Serial.println("File renamed");
    return DATABASE_SUCCESS_RENAME_FILE;
  }
  
  Serial.println("Rename failed");

  return DATABASE_ERROR_RENAME_FILE;
}

bool IOT_database::delete_file(fs::FS &fs, const char * path) 
{
  Serial.printf("Deleting file: %s\n", path);

  if(fs.remove(path))
  {
    Serial.println("File deleted");

    return DATABASE_SUCCESS_DELETE_FILE;
  }

  Serial.println("Delete failed");

  return DATABASE_ERROR_DELETE_FILE;
}

bool IOT_database::delete_line(fs::FS &fs, const char * path, int line)
{
  Serial.printf("Delete a line on file: %s\n", path);

  int currentLine = 1, count = 8;
  String tempPath = "/temp.txt";
  String tempStorage = "";
  File file = fs.open(path);

  if(!file) 
  {
    Serial.println("Failed to open file for deleting line");
    return DATABASE_ERROR_FAILED_OPEN_FILE;
  }

  while(file.available())
  {
    char c = file.read();
    if (c == '\n')
    {
      Serial.println();

      if (line != currentLine)
        tempStorage += c;

      currentLine++;
    }
    else 
    {
      if (line != currentLine) 
      {
        Serial.print(c);
        tempStorage += c;
      }
    }
  }

  file.close();

  while ((!file) && (--count))
  {
    file = fs.open(tempPath.c_str(), FILE_WRITE);
    if(!file)
    {
      Serial.println("File doesn't exist");
      Serial.println("Creating file...");
    }
    else
    {
      Serial.println("File already exists");
      break;
    }
    vTaskDelay(20/portTICK_PERIOD_MS);
  }

  file.close();

  if (!(tempStorage == ""))
  {
    if (this->write_file(fs, tempPath.c_str(), tempStorage.c_str()) == DATABASE_ERROR_FAILED_WRITE_FILE)
    {
      return DATABASE_ERROR_FAILED_WRITE_FILE;
    }
  }
  
  if (this->delete_file(fs, path) == DATABASE_ERROR_DELETE_FILE) 
  {
    return DATABASE_ERROR_DELETE_FILE;
  }

  if (this->rename_file(fs, tempPath.c_str(), path) == DATABASE_ERROR_RENAME_FILE) 
  {
    return DATABASE_ERROR_RENAME_FILE;
  }

  return DATABASE_SUCCESS_DELETE_FILE;
}

int IOT_database::get_number_of_line(fs::FS &fs, const char * path, int *line)
{
  Serial.print("Reading number of line on file: ");
  Serial.println(path);
  *line = 0;
  // int currentLine = 1;
  File file = fs.open(path);
  if(!file) 
  {
    Serial.println("Failed to open file for reading");
    return DATABASE_ERROR_FAILED_OPEN_FILE;
  }
  // lineStorage = "";
  while(file.available()) 
  {
    char c = file.read();

    if (c == '\n') 
    {
      *line += 1;
    }
  }

  file.close();

  return DATABASE_SUCCESS_READ_FILE;
}

bool IOT_database::read_line(fs::FS &fs, const char * path, char *lineStorage, int lineNumber)
{
  Serial.print("Reading a line on file: ");
  Serial.println(path);

  int currentLine = 1, i = 0;
  File file = fs.open(path);
  if(!file) 
  {
    Serial.println("Failed to open file for deleting line");
    return DATABASE_ERROR_FAILED_OPEN_FILE;
  }

  // lineStorage = "";

  while(file.available()) 
  {
    char c = file.read();

    if (c == '\n') 
    {
      // Serial.println();
      currentLine++;
    }
    else 
    {
      if (lineNumber == currentLine) 
      {
        // Serial.print(c);
        lineStorage[i] = c;
        i++;
      }
      else if (lineNumber < currentLine) break;
    }
  }

  file.close();

  return DATABASE_SUCCESS_READ_FILE;
}

bool IOT_database::read_line(fs::FS &fs, const char * path, String &lineStorage, int lineNumber)
{
  Serial.print("Reading a line on file: ");
  Serial.println(path);

  int currentLine = 1;
  File file = fs.open(path);
  if(!file) 
  {
    Serial.println("Failed to open file for deleting line");
    return DATABASE_ERROR_FAILED_OPEN_FILE;
  }

  lineStorage = "";

  while(file.available()) 
  {
    char c = file.read();

    if (c == '\n') 
    {
      // Serial.println();
      currentLine++;
    }
    else 
    {
      if (lineNumber == currentLine) 
      {
        // Serial.print(c);
        lineStorage += c;
      }
      if (lineNumber < currentLine) break;
    }
  }

  file.close();

  return DATABASE_SUCCESS_READ_FILE;
}

// void testFileIO(fs::FS &fs, const char * path){
//   File file = fs.open(path);
//   static uint8_t buf[512];
//   size_t len = 0;
//   uint32_t start = millis();
//   uint32_t end = start;
//   if(file){
//     len = file.size();
//     size_t flen = len;
//     start = millis();
//     while(len){
//       size_t toRead = len;
//       if(toRead > 512){
//         toRead = 512;
//       }
//       file.read(buf, toRead);
//       len -= toRead;
//     }
//     end = millis() - start;
//     Serial.printf("%u bytes read for %u ms\n", flen, end);
//     file.close();
//   } else {
//     Serial.println("Failed to open file for reading");
//   }


//   file = fs.open(path, FILE_WRITE);
//   if(!file){
//     Serial.println("Failed to open file for writing");
//     return;
//   }

//   size_t i;
//   start = millis();
//   for(i=0; i<2048; i++){
//     file.write(buf, 512);
//   }
//   end = millis() - start;
//   Serial.printf("%u bytes written for %u ms\n", 2048 * 512, end);
//   file.close();
// }