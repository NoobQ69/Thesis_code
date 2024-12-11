#ifndef SPI_FFS_UTILITIES_H
#define SPI_FFS_UTILITIES_H

// Initialize SPIFFS
int initSPIFFS() 
{
  if (!SPIFFS.begin(true)) 
  {
    Serial.println("An error has occurred while mounting SPIFFS");
    return -1;
  }
  Serial.println("SPIFFS mounted successfully");
  return 1;
}

// Read File from SPIFFS
int readFile(fs::FS &fs, const char * path, char *str)
{
  int i = 0;
  Serial.printf("Reading file: %s\r\n", path);

  File file = fs.open(path);
  if(!file || file.isDirectory())
  {
    Serial.println("- failed to open file for reading");
    return -1;
  }
  
  // String fileContent;
  while(file.available())
  {
    // fileContent = file.readStringUntil('\n');
    str[i] = file.read();
    i++;
  }
  file.close();
  return 1;
}

// Write file to SPIFFS
int writeFile(fs::FS &fs, const char * path, const char * message)
{
  Serial.printf("Writing file: %s\r\n", path);

  File file = fs.open(path, FILE_WRITE);
  
  if(!file)
  {
    Serial.println("- failed to open file for writing");
    return -1;
  }
  
  if(file.print(message))
  {
    Serial.println("- file written");
    file.close();
    return 1;
  } 
  else {
    Serial.println("- write failed");
    file.close();
    return 0;
  }
}

#endif