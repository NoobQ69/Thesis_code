#ifndef CMD_UTILITIES_H_
#define CMD_UTILITIES_H_

// void process_cmd_from_serial();
void split_string_char(const char *data, char separator, int index, char string_to_store[]);
int str_startswith(const char * string_compare,  const char * string_to_compare);
#endif
