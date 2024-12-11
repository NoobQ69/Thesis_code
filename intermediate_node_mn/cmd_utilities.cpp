#include "cmd_utilities.h"
#include "services/RoutingTableService.h"

void split_string_char(const char *data, char separator, int index, char string_to_store[]) 
{
  int found = 0;
  int strIndex[] = { 0, -1 };
  int maxIndex = strlen(data) - 1;
  
  for (int i = 0; i <= maxIndex && found <= index; i++) 
  {
    if (data[i] == separator || i == maxIndex) 
    {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }

  if (found > index)
  {
    memset(string_to_store, 0, strIndex[1] - strIndex[0] + 1);
    for (int i = 0; i < strIndex[1] - strIndex[0]; i++)
    {
      string_to_store[i] = data[i+strIndex[0]];
    }
  }
}

int str_startswith(const char * string_compare,  const char * string_to_compare)
{
  int len = 0;
  int len1 = strlen(string_compare);
  int len2 = strlen(string_to_compare);
  
  if (len1 < len2) len = len1;
  else len = len2;

  for (int i = 0; i < len; i++)
  {
    if (string_compare[i] != string_to_compare[i]) return 0;
  }

  return 1;
}