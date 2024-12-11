// #define ESP8266
#include "LoraMesher.h"
#include "cmd_utilities.h"
//Using LILYGO TTGO T-BEAM v1.1 
#define BOARD_LED   2
#define LED_ON      LOW
#define LED_OFF     HIGH

LoraMesher& radio = LoraMesher::getInstance();

char Buffer[50];

uint32_t dataCounter = 0;
struct dataPacket {
  char data_str[50];
};

dataPacket* helloPacket = new dataPacket;

//Led flash
void led_Flash(uint16_t flashes, uint16_t delaymS) {
    uint16_t index;
    for (index = 1; index <= flashes; index++) {
        digitalWrite(BOARD_LED, LED_ON);
        delay(delaymS);
        digitalWrite(BOARD_LED, LED_OFF);
        delay(delaymS);
    }
}

/**
 * @brief Print the counter of the packet
 *
 * @param data
 */
void printPacket(dataPacket data) {
    Serial.printf("Hello Counter received: %s\n", data.data_str);
}

/**
 * @brief Iterate through the payload of the packet and print the counter of the packet
 *
 * @param packet
 */
void printDataPacket(AppPacket<dataPacket>* packet) {
    Serial.printf("Packet arrived from %X with size %d\n", packet->src, packet->payloadSize);

    //Get the payload to iterate through it
    dataPacket* dPacket = packet->payload;
    size_t payloadLength = packet->getPayloadLength();

    for (size_t i = 0; i < payloadLength; i++) {
        //Print the packet
        printPacket(dPacket[i]);
    }
}

void get_routing_node_address() {

    //Set the routing table list that is being used and cannot be accessed (Remember to release use after usage)
    LM_LinkedList<RouteNode>* routingTableList = radio.routingTableListCopy();

    routingTableList->setInUse();

    // Screen.changeSizeRouting(radio.routingTableSize());

    char text[15];
    for (int i = 0; i < radio.routingTableSize(); i++) {
        RouteNode* rNode = (*routingTableList)[i];
        NetworkNode node = rNode->networkNode;
        snprintf(text, 15, ("|%X(%d)->%X"), node.address, node.metric, rNode->via);
        // Screen.changeRoutingText(text, i);
    }

    //Release routing table list usage.
    routingTableList->releaseInUse();

    //Delete the routing table list
    delete routingTableList;

    // Screen.changeLineFour();
}

/**
 * @brief Function that process the received packets
 *
 */
void processReceivedPackets(void*) {
    for (;;) {
        /* Wait for the notification of processReceivedPackets and enter blocking */
        ulTaskNotifyTake(pdPASS, portMAX_DELAY);
        led_Flash(1, 100); //one quick LED flashes to indicate a packet has arrived

        //Iterate through all the packets inside the Received User Packets Queue
        while (radio.getReceivedQueueSize() > 0) {
            Serial.println("ReceivedUserData_TaskHandle notify received");
            Serial.printf("Queue receiveUserData size: %d\n", radio.getReceivedQueueSize());

            //Get the first element inside the Received User Packets Queue
            AppPacket<dataPacket>* packet = radio.getNextAppPacket<dataPacket>();
            led_Flash(3, 200);
            //Print the data packet
            printDataPacket(packet);

            // if (packet->dst == BROADCAST_ADDR)
            // {
            // }
            // else
            // {
            //   Serial.printf("Send packet %d\n", dataCounter);
            //   memset(helloPacket->data_str, 0, 50);
            //   strcpy(helloPacket->data_str, "Receive successfully");
            //   radio.createPacketAndSend(packet->src, helloPacket, 1);
            // }

            String string_data_str = String(helloPacket->data_str);
            
            if(string_data_str.indexOf("SEND_CMD") > 0)
            {
              Serial.println("Jump here?");
              int pos = strlen("SEND_CMD");
              char temp_str[30];
              memset(temp_str, 0, 30);
              strncpy(temp_str,helloPacket->data_str+(pos+1),strlen(helloPacket->data_str));
              Serial.println(temp_str);
              Serial2.println(temp_str);
            }
            //Create packet and send it.
            // radio.createPacketAndSend(BROADCAST_ADDR, helloPacket, 1);
            //Delete the packet when used. It is very important to call this function to release the memory of the packet.
            radio.deletePacket(packet);
        }
    }
}

TaskHandle_t receiveLoRaMessage_Handle = NULL;

/**
 * @brief Create a Receive Messages Task and add it to the LoRaMesher
 *
 */
void createReceiveMessages() {
    int res = xTaskCreate(
        processReceivedPackets,
        "Receive App Task",
        4096,
        (void*) 1,
        2,
        &receiveLoRaMessage_Handle);
    if (res != pdPASS) {
        Serial.printf("Error: Receive App Task creation gave error: %d\n", res);
    }
}

SPIClass newSPI(VSPI);
/**
 * @brief Initialize LoRaMesher
 *
 */
void setupLoraMesher() {
    //Get the configuration of the LoRaMesher
    pinMode(2, OUTPUT);
    LoraMesher::LoraMesherConfig config = LoraMesher::LoraMesherConfig();

    //Set the configuration of the LoRaMesher (TTGO T-BEAM v1.1)
    // config.loraCs = 2;
    // config.loraRst = 4;
    // config.loraIrq = 15;
    // config.loraIo1 = 5;

    // config.loraCs = 5;
    // config.loraRst = 4;
    // config.loraIrq = 2;
    // config.loraIo1 = 15;
    // config.loraCs = 2;
    
    config.loraCs = 13;
    config.loraRst = 4;
    config.loraIrq = 15;
    config.loraIo1 = 5;

    newSPI.begin();
    config.spi = &newSPI; //sck, miso, mosi

    config.freq = 434.0;

    config.module = LoraMesher::LoraModules::SX1278_MOD;

    //Init the loramesher with a configuration
    radio.begin(config);

    //Create the receive task and add it to the LoRaMesher
    createReceiveMessages();

    //Set the task handle to the LoRaMesher
    radio.setReceiveAppDataTaskHandle(receiveLoRaMessage_Handle);

    //Start LoRaMesher
    radio.start();

    Serial.println("Lora initialized");
}

void process_cmd_from_LoRa()
{

}

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
    else if (process_cmd.startsWith("SEND_BROADCAST"))       // need revise, cmd to test sending dummy data to a specific node, SEND_TO,(node address)
    {
      strcpy(helloPacket->data_str, "Hello broadcast");
      // //Create packet and send it.
      radio.createPacketAndSend(BROADCAST_ADDR, helloPacket, 1);
    }
    else if (process_cmd.startsWith("SEND_TO"))       // need revise, cmd to test sending dummy data to a specific node, SEND_TO,(node address)
    {
      // strcpy(helloPacket->data_str, "Hello from 6A08");
      char addr[10];
      char cmd[30];
      memset(addr, 0, 10);
      split_string_char(process_cmd.c_str(), ',', 1, addr);
      // split_string_char(process_cmd.c_str(), ',', 2, cmd);
      uint16_t hex_int = strtol(addr, NULL, 16);

      strcpy(helloPacket->data_str, process_cmd.substring(13, process_cmd.length()).c_str());
      Serial.println(helloPacket->data_str);
      // //Create packet and send it.
      radio.createPacketAndSend(hex_int, helloPacket, 1);
    }
    // else if (str_startswith(helloPacket->data_str, "SEND_CMD"))
    else if (process_cmd.startsWith("SEND_CMD"))
    {
      char data_str[30];
      memset(data_str, 0, 30);
      strcpy(data_str, process_cmd.c_str());

      int pos = strlen("SEND_CMD");
      char temp_str[30];
      memset(temp_str, 0, 30);
      strncpy(temp_str,data_str+(pos+1),strlen(data_str));
      Serial.println(temp_str);
      Serial2.println(temp_str);
    }
  }
}

void setup() {
    Serial.begin(115200);
    Serial2.begin(115200);

    Serial.println("initBoard");
    pinMode(BOARD_LED, OUTPUT); //setup pin as output for indicator LED
    led_Flash(2, 125);          //two quick LED flashes to indicate program start
    setupLoraMesher();
}

int curr_time, pre_time;
void loop() {
  curr_time = millis();
  if (curr_time - pre_time > 5000)
  {
    pre_time = curr_time;
    // Serial.printf("Send packet %d\n", dataCounter);

    // strcpy(helloPacket->data_str, "Hello from 6A08");

    // //Create packet and send it.
    // radio.createPacketAndSend(BROADCAST_ADDR, helloPacket, 1);

    //Wait 20 seconds to send the next packet
    // vTaskDelay(5000 / portTICK_PERIOD_MS);
  }
  process_cmd_from_serial();
    // for (;;) {
    // }
}