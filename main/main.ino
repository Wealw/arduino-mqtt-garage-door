#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <curveFitting.h>

#define relay 5
#define trigPin 6
#define echoPin 7

byte mac[] = { 0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02 };
IPAddress broker(10,0,30,100);
int        port     = 1883;
const char topic[]  = "garage_door_status";
const char username[] = "homeassistant";
const char password[] = "ohGae1waiquo0hohthie5xa9AiThovu3ohtheip9ohohj6teingo7soh7oe1ahdi";
double distanceBuffer[20];

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
  pinMode(relay, OUTPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  while(!ipConnect()){}
  while(!mqttConnect()){}
  client.publish("garage_door_command","idling");
  client.publish("garage_door_status","opened");
}

void loop()
{
  client.loop();
  delay(1000);
}

void openGarageDoor(){
  Serial.println("Opening garage door...");
  client.publish("garage_door_command","idling");
  if(!opened){
    client.publish("garage_door_status","moving");
    activateGarageDoor();
    if (isMovingDown()){
      Serial.println("Door is moving wrong way, reversing...");
      activateGarageDoor();
      delay(500);
      activateGarageDoor();
    }
    delay(4000);
    if (doorIsOpened()){
      markDoorOpened();
    } else {
     openGarageDoor();
    }
  } else {
    Serial.println("Door already in good state");
  }
}

void closeGarageDoor(){
  Serial.println("Closing garage door...");
  client.publish("garage_door_command","idling");
  if(opened){
    client.publish("garage_door_status","moving");
    activateGarageDoor();
    if (isMovingUp()){
      Serial.println("Door is moving wrong way, reversing...");
      activateGarageDoor();
      delay(500);
      activateGarageDoor();
    }
    delay(4000);
    if (doorIsClosed()){
      markDoorClosed();
    } else {
      closeGarageDoor();
    }
  } else {
    Serial.println("Door already in good state");
  }
}


double mesureDistance(){
  delay(5);
  digitalWrite(trigPin, LOW);
  delay(10);
  digitalWrite(trigPin, HIGH);
  delay(50);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH);
  long temp = duration * 0.034 / 2;
  if (temp >= 12) {temp = 14;}
  if (temp <= 6) {temp = 6;}
  return temp;
}


void activateGarageDoor() {
  Serial.println("Activating relay...");
  digitalWrite(relay, LOW);
  delay(5);
  digitalWrite(relay, HIGH);
  delay(2000);
  digitalWrite(relay, LOW);
  delay(5);
}

bool isMovingUp(){
  fillDistanceBuffer();
  int temp = linearRegression();
  if (temp > 0) {
    Serial.println("Door is moving up !");
    return true;
  }
  return false;
}

bool isMovingDown(){
  fillDistanceBuffer();
  int temp = linearRegression();
  if (temp < 0) {
    Serial.println("Door is moving down !");
    return true;
  }
  return false;
}

bool doorIsClosed(){
  fillDistanceBuffer();
  int temp = linearRegression();
  if (temp > -0.1 && temp < 0.1) {
    int avg = average();
    if ( avg > 5.0 && avg < 8.0){
      return true;
    }
  }
  return false;
}

bool doorIsOpened(){
  fillDistanceBuffer();
  int temp = linearRegression();
  if (temp > -0.1 && temp < 0.1) {
    double avg = average();
    if ( avg > 11.0 && avg < 16.0){
      return true;
    }
  }
  return false;
}

double average(){
  double temp = 0;
  for (int i =0; i < 20 ; i++){
    temp = temp + distanceBuffer[i];
  }
  Serial.print("Average buffer value is :");
  Serial.println(temp/20);
  return temp / 20;
}

double linearRegression(){
  double coeffs[2];
  fitCurve(1, 20, distanceBuffer, 2, coeffs);
  Serial.print("Linear regression factor is :");
  Serial.print(coeffs[0]);
  Serial.print(":");
  Serial.println(coeffs[0]);
  return coeffs[0];
}

void fillDistanceBuffer(){
  for(int i = 0; i < 20 ; i++){
    delay(300);
    distanceBuffer[i] = mesureDistance();
  }
}

void markDoorOpened() {
    client.publish("garage_door_status","opened");
    opened = true;
    Serial.println("Garage door is opened !");
}

void markDoorClosed() {
    client.publish("garage_door_status","closed");
    opened = false;
    Serial.println("Garage door is closed !");
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
