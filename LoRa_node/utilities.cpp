#include "utilities.h"


int map_value(int value, int fromLow, int fromHigh, int toLow, int toHigh) 
{
  // Handle edge cases
  if (value <= fromLow) return toLow;
  if (value >= fromHigh) return toHigh;
  // Calculate the mapped value
  return toLow + (value - fromLow) * (toHigh - toLow) / (fromHigh - fromLow);
}

int str_startswith(const char * string_compare,  const char * string_to_compare)
{
  int len = 0;
  int len1 = strlen(string_compare);
  int len2 = strlen(string_to_compare);
  
  if (len1 == 0 || len2 == 0) return 0;

  if (len1 < len2) len = len1;
  else len = len2;

  for (int i = 0; i < len; i++)
  {
    if (string_compare[i] != string_to_compare[i]) return 0;
  }

  return 1;
}

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

int convert_string_to_int(const char *string_number, int len)
{
  int number = 0;
  for (int i = 0; i < len; i++)
  {
    if (string_number[i] >= 0x30 && string_number[i] <= 0x39)
      number = (number * 10) + (string_number[i] - 0x30);
  }
  return number;
}

void convert_int_to_str(int N, char *str) 
{
  if (N == 0)
  {
    str[0] = '0';
    return;
  }
  int i = 0;
  int sign = N;

  if (N < 0)
      N = -N;

  // Extract digits from the number and add them to the
  // string
  while (N > 0) {
    
      // Convert integer digit to character and store
      // it in the str
      str[i++] = N % 10 + '0';
      N /= 10;
  } 

  // If the number was negative, add a minus sign to the
  // string
  if (sign < 0) {
      str[i++] = '-';
  }

  // Null-terminate the string
  str[i] = '\0';

  // Reverse the string to get the correct order
  for (int j = 0, k = i - 1; j < k; j++, k--)
  {
      char temp = str[j];
      str[j] = str[k];
      str[k] = temp;
  }
}

int count_character(const char *string, int len, char char_indicator)
{
  int count = 0;
  for (int i = 0; i < len; i++)
  {
    if (string[i] == char_indicator)
    {
      count++;
    }
  }
  return count;
}

/**
 * @brief 
 *
 * @param str : string contains character
 * @param c : character to get index from a string
 * @param position : if there are more than 1 character that are in a string
 */
int get_index_from_string(const char *str, char c, int position)
{
  int i, curr_position = 0, len = strlen(str);
  
  if (len == 0) return -1;

  for (i = 0; i < len; i++)
  {
    if (str[i] == c)
    {
      if (curr_position == position)
      {
        return i;
      }
      else
      {
        curr_position++;
      }
    }
  }
  return -1;
}


