#include <Ethernet.h>
#include <PubSubClient.h>
#include "secret.h"

/* Global variable declaration */
double distanceBuffer[buffer_size];
int cursor = 0;
int networkLimiter = 0;
bool previousOpenStatus = false;

/* Handle subscribed topic different values, declared before static declaration */
void callback(char *topic, byte *payload, unsigned int length) {
    payload[length] = '\0';
    if (strcmp((char *) topic, command_topic) == 0) {
        if (strcmp((char *) payload, "open") == 0) {
            activateGarageDoor();
        } else if (strcmp((char *) payload, "close") == 0) {
            activateGarageDoor();
        } else if (strcmp((char *) payload, "stop") == 0) {
            activateGarageDoor();
        }
    }
}

EthernetClient ethernetClient;
PubSubClient client(broker, port, callback, ethernetClient);

/* Function that measure distance from an ultrasound sensor*/
double measureDistance() {
    delay(5);
    digitalWrite(trigPin, LOW);
    delay(10);
    digitalWrite(trigPin, HIGH);
    delay(50);
    digitalWrite(trigPin, LOW);
    long duration = pulseIn(echoPin, HIGH);
    long temp = duration * 0.034 / 2;
    if (temp >= 12) { temp = 14; }
    if (temp <= 6) { temp = 6; }
    return temp;
}

void activateGarageDoor() {
    digitalWrite(relay, LOW);
    delay(5);
    digitalWrite(relay, HIGH);
    delay(300);
    digitalWrite(relay, LOW);
    delay(5);
}

void clientResub(){
    client.unsubscribe(command_topic);
    client.subscribe(command_topic);
}

double average() {
    double temp = 0;
    for (int i = 0; i < buffer_size; i++) {
        temp = temp + distanceBuffer[i];
    }
    return temp / buffer_size;
}

bool ipConnect() {
    IPAddress current = Ethernet.localIP();
    if (!((current == IPAddress(0, 0, 0, 0)) || (current == IPAddress(255, 255, 255, 255))))
      return true;

    if (Ethernet.begin(mac) != 0)
        return true;

    if (Ethernet.hardwareStatus() == EthernetNoHardware)
        while (1);

    if (Ethernet.linkStatus() == LinkOFF) {
        delay(10000);
        return false;
    }
}

bool mqttConnect() {
    if (!client.connected()){
      client.connect("garagedoor_cxJ&b4", username, password);
      client.subscribe(command_topic);
    }

    if (client.connected())
      return true;

    delay(10000);
    return false;
}

void setup() {
    pinMode(relay, OUTPUT);
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);

    for (int i = 0; i < buffer_size; i++) {
        distanceBuffer[i] = measureDistance();
    }
}

void loop() {
    while (!ipConnect()) {}
    while (!mqttConnect()) {}

    distanceBuffer[cursor] = measureDistance();
    cursor = (cursor + 1) % buffer_size;

    if (average() > 13.0 ) {
      if (previousOpenStatus == true || networkLimiter == 0)
        client.publish(state_topic, "closed");
      previousOpenStatus = false;
    } else {
      if (previousOpenStatus == false || networkLimiter == 0)
        client.publish(state_topic, "open");
      previousOpenStatus = true;
    }

    client.loop();

    if (networkLimiter == 0) {
      client.publish(status_topic, status_state_online);
    }

    networkLimiter = (networkLimiter + 1) % 30;
    delay(polling_rate_delay);
}
