#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>

byte mac[] = { 0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02 };
IPAddress broker(10,0,30,100);
int        port     = 1883;
const char topic[]  = "garage_door_status";
const char username[] = "homeassistant";
const char password[] = "ohGae1waiquo0hohthie5xa9AiThovu3ohtheip9ohohj6teingo7soh7oe1ahdi";

const long interval = 8000;
unsigned long previousMillis = 0;

bool opened = true;


void callback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0';
  if (strcmp((char *)topic, "garage_door_command") == 0) {
    if (strcmp((char *)payload, "open") == 0){
      Serial.println("Opening door");
      openGarageDoor();
    }
    else if (strcmp((char *)payload, "close") == 0) {
      Serial.println("Closing door");
      closeGarageDoor();
    }else if (strcmp((char *)payload, "idling") == 0) {
      Serial.println("Idling...");
    } else {
      Serial.println("Invalid command received");
    }
  }
}

EthernetClient ethernetClient;
PubSubClient client(broker, 1883, callback, ethernetClient);

void setup()
{
  Serial.begin(9600);
  while(!ipConnect()){}
  while(!mqttConnect()){}
}

void loop()
{
  client.loop();
  delay(1000);
}

void openGarageDoor(){
  client.publish("garage_door_command","idling");
  if(!opened){
    client.publish("garage_door_status","moving");
        activateGarageDoor();
    if (!isMovingUp()){
      activateGarageDoor();
      delay(500);
      activateGarageDoor();
    }
    delay(4000);
    if (doorIsUp()){
      markDoorOpened();
    }
  } else {
    Serial.println("Door already in good state");
  }
}

void closeGarageDoor(){
  client.publish("garage_door_command","idling");
  if(opened){
    client.publish("garage_door_status","moving");
    activateGarageDoor();
    if (isMovingUp()){
      activateGarageDoor();
      delay(500);
      activateGarageDoor();
    }
    delay(4000);
    if (!doorIsUp()){
          markDoorClosed();
    }
  } else {
    Serial.println("Door already in good state");
  }
}



void activateGarageDoor() {
  //TODO
}

bool isMovingUp(){
  //TODO
}

bool doorIsUp(){
  //TODO
}

void markDoorOpened() {
    client.publish("garage_door_status","opened");
    opened = true;
}

void markDoorClosed() {
    client.publish("garage_door_status","closed");
    opened = false;
}

bool ipConnect()
{
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
      while(1);
    } else if (Ethernet.linkStatus() == LinkOFF) {
      Serial.println("Ethernet cable is not connected.");
    }
    Serial.println("Retrying in 10 seconds...");
    delay(10000);
    return false;
  } else {
    Serial.print("Ethernet shield setup successfully with IP: " );
    Serial.println(Ethernet.localIP());
    return true;
  }
  return false;
}

bool mqttConnect()
{
  if (!client.connect("arduinoClient", username, password)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(client.state());
    Serial.println("Retrying in 10 seconds...");
    delay(10000);
    return false;
  } else {
    Serial.println("Connected to the MQTT broker!");
    client.subscribe("garage_door_command");
    return true;
  }
  return false;
}
