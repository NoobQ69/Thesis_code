#include <SPI.h>
#include "lora_star_topo.h"
#include <LoRa.h>

int counter = 0;
LoraStar Node1;
char buffer[250];
void handle_receive_user_packet(void *param)
{
  // LoraStar *lora_obj = (LoraStar*)params;
  BaseType_t TWres = pdPASS;
  while (1)
  {
    Serial.println("User waiting data receive");
    TWres = xTaskNotifyWait(pdTRUE, pdFALSE, NULL, portMAX_DELAY);

    if (TWres == pdPASS)
    {
      while (!User_receive_queue.is_empty())
      {
        User_receive_queue.get_payload_head(buffer);
        int pos = get_index_from_string(buffer, '&', PACKET_PAYLOAD_POSITION-1);
        Serial.printf("Packet received, content is: %s\n",buffer+pos+1);
        User_receive_queue.delete_head();
        // lora_obj->send_data();
      }
    }
  }
}

void setup() {
  Serial.begin(9600);
  Node1._user_receive_packet_func = handle_receive_user_packet;
  // Node1._role = SINK_NODE;
  // Node1.begin();
  Node1._role = USER_NODE;

  Node1.begin();
  while (!Serial);

  Serial.println("LoRa Sender");

  // if (!LoRa.begin(433E6)) {
  //   Serial.println("Starting LoRa failed!");
  //   while (1);
  // }
}


LoRa_data_packet_t Lora_packet;

void loop() 
{
  if (Serial.available())
  {
    int i = 0;
    memset(buffer, 0, 250);
    delay(50);
    while (Serial.available())
    {
      char c = Serial.read();
      buffer[i] = c;
      i++;
    }
    Serial.println(buffer);

    Lora_packet.source_addr = Node1.get_local_address();
    Lora_packet.destination_addr = 0x745c;
    Lora_packet.size = strlen(buffer);
    Lora_packet.packet_num = 0;
    Lora_packet.type = 1;
    Lora_packet.payload = buffer;
    Node1.create_packet_data_and_send(&Lora_packet);
  }
  // Serial.print("Sending packet: ");
  // Serial.println(counter);

  // // send packet
  // LoRa.beginPacket();
  // LoRa.print("this_string_is_very_long_and_I_want_to_send_it_to_other_lora_devices_to_notice_to_this_device_that_the_current_number_is:");
  // LoRa.print(counter);
  // LoRa.endPacket();

  // counter++;

  // delay(5000);
}
