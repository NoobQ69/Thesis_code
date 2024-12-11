#include "cmd_utilities.h"
#include "services/RoutingTableService.h"

void process_cmd_from_serial()
{
  String process_cmd = "";
  if (Serial.available())
  {
    char c;
    while (Serial.available())
    {
      c = Serial.read();
      process_cmd += c;
    }

    if (process_cmd.startsWith("PRINT_ROUTING_TABLE"))
    {
      RoutingTableService::printRoutingTable();
    }
  }
}