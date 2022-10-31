#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <curveFitting.h>

// Pin configuration
#define relay 5
#define trigPin 6
#define echoPin 7

// IP configuration
byte mac[] = { 0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02 };

// MQTT configuration
#define username "homeassistant"
#define password "ohGae1waiquo0hohthie5xa9AiThovu3ohtheip9ohohj6teingo7soh7oe1ahdi"
#define port 1883
const IPAddress broker(10,0,30,100);

// Topic configuration
#define state_topic "state/garage_door_1"
#define command_topic "command/garage_door_1"
#define status_topic "telemetry/garage_door_1"

// Topic value configuration
#define status_state_online "online"
#define max_open_time 18000
#define sample_time 15

// Global variable declaration
double distanceBuffer[20];
bool opened = true;

/* Handle subscribed topic different values, declared before static declaration*/
void callback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0';
  if (strcmp((char *)topic, command_topic) == 0) {
    if (strcmp((char *)payload, "open") == 0){
      openGarageDoor();
    }
    else if (strcmp((char *)payload, "close") == 0) {
      closeGarageDoor();
    }else if (strcmp((char *)payload, "stop") == 0) {
      stopGarageDoor();
    } else {
    }
  }
}

// IP and MQTT client instaciation
EthernetClient ethernetClient;
PubSubClient client(broker, port, callback, ethernetClient);


/*
* #########################################################
* Command handlers
* #########################################################
*/

void openGarageDoor(){
  Serial.println("[LOG] Opening command received");
  fillDistanceBuffer();
  if(!opened && linearRegression() == 0.0){
    Serial.println("[LOG] Opening garage door");
    activateGarageDoor();
    markDoorOpened();
    delay(700);
    if (isMovingDown()){
      //Serial.println("Door is moving wrong way, reversing...");
      activateGarageDoor();
      delay(500);
      activateGarageDoor();
    }
    //delay(max_open_time);
  } else {
    Serial.println("[LOG] Door already open or moving");
  }
}

void closeGarageDoor(){
  Serial.println("[LOG] Closing command received");
  fillDistanceBuffer();
    if(opened && linearRegression() == 0.0){
      Serial.println("[LOG] Closing garage door");
      activateGarageDoor();
      delay(700);
      if (isMovingUp()){
        //Serial.println("Door is moving wrong way, reversing...");
        activateGarageDoor();
        delay(500);
        activateGarageDoor();
      }
      //delay(max_open_time);
    } else {
      Serial.println("[LOG] Door already closed or moving");
    }

}

void stopGarageDoor(){
  Serial.println("[LOG] Stoping command received");
  if (isMoving() == true ){
    Serial.println("[LOG] Stopping garage door");
    activateGarageDoor();
  } else {
    Serial.println("[LOG] Door already stopped");
  }
}

/*
* #########################################################
* State evaluation functions
* #########################################################
*/

bool isMovingUp(){
  fillDistanceBuffer();
  double temp = 0;
  for (int i =0; i < 19 ; i++){
    temp = temp + ((distanceBuffer[i +1] - distanceBuffer[i])/1);
  }
    Serial.print("Moving up factor is :");
  Serial.println(temp);
  if (temp > 0){
    return true;
  } else {
    return false;
  }
}

bool isMoving(){
  fillDistanceBuffer();
  double temp = 0;
  for (int i =0; i < 19 ; i++){
    temp = temp + abs(distanceBuffer[i +1] - distanceBuffer[i]);
  }
  if (temp > 0){
    return true;
  } else {
    return false;
  }
}

bool isMovingDown(){
  fillDistanceBuffer();
  double temp = 0;
  for (int i =0; i < 19 ; i++){
    temp = temp + ((distanceBuffer[i +1] - distanceBuffer[i])/1);
  }
  Serial.print("Moving down factor is :");
  Serial.println(temp);
  if (temp < 0){
    return true;
  } else {
    return false;
  }
}

bool doorIsOpened(){
fillDistanceBuffer();
double avg = average();
if ( avg < 13.0){
  return true;
}
return false;
}

void checkDoorStatus(){
if (doorIsOpened() == true){
  markDoorOpened();
} else {
  markDoorClosed();
}
}


/*
* #########################################################
* Captor
* #########################################################
*/

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
  delay(500);
  digitalWrite(relay, LOW);
  delay(5);
}


double average(){
  double temp = 0;
  for (int i =0; i < 20 ; i++){
    temp = temp + distanceBuffer[i];
  }
  //Serial.print("Average buffer value is :");
  //Serial.println(temp/20);
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
    delay(sample_time);
    distanceBuffer[i] = mesureDistance();
  }
}

void markDoorOpened() {
    opened = true;
    Serial.println("[LOG] Garage open !");
}

void markDoorClosed() {
    opened = false;
    Serial.println("[LOG] Garage closed !");
}

bool ipConnect()
{
  IPAddress current = Ethernet.localIP();
  if((current == IPAddress(0,0,0,0)) || (current == IPAddress(255,255,255,255))){
    if (Ethernet.begin(mac) == 0) {
      if (Ethernet.hardwareStatus() == EthernetNoHardware) {
        while(1);
      } else if (Ethernet.linkStatus() == LinkOFF) {
      }
      delay(10000);
      return false;
    } else {
      return true;
    }
  }
  return true;
}

bool mqttConnect()
{
  if (!client.connect("arduinoClient", username, password)) {
    delay(10000);
    return false;
  } else {
    client.subscribe(command_topic);
    return true;
  }
  return false;
}



void setup()
{
  Serial.begin(9600);
  pinMode(relay, OUTPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  while(!ipConnect()){}
  while(!mqttConnect()){}
}

void loop()
{
  while(!ipConnect()){}
  while(!mqttConnect()){}
  client.publish(status_topic, status_state_online);
  client.loop();
  checkDoorStatus();
  if (opened){
        client.publish(state_topic,"open");
  } else {
        client.publish(state_topic,"closed");
  }
}
