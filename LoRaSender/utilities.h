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

int count_character(const char *string, char char_indicator)
{
  int count = 0;
  int len = strlen(string);
  for (int i = 0; i < len; i++)
  {
    if (string[i] == char_indicator)
    {
      count++;
    }
  }
  return count;
}

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
