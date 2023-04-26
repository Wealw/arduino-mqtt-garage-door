#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include "secret.h"

/* Global variable declaration */
double distanceBuffer[buffer_size];
bool opened = true;
int cursor = 0;

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
        } else {
            // Do nothing
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

double average() {
    double temp = 0;
    for (int i = 0; i < buffer_size; i++) {
        temp = temp + distanceBuffer[i];
    }
    return temp / buffer_size;
}

bool ipConnect() {
    IPAddress current = Ethernet.localIP();
    if ((current == IPAddress(0, 0, 0, 0)) || (current == IPAddress(255, 255, 255, 255))) {
        if (Ethernet.begin(mac) == 0) {
            if (Ethernet.hardwareStatus() == EthernetNoHardware) {
                while (1);
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

bool mqttConnect() {
    if (!client.connect("arduinoClient", username, password)) {
        delay(10000);
        return false;
    } else {
        client.subscribe(command_topic);
        return true;
    }
    return false;
}

void setup() {
    pinMode(relay, OUTPUT);
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);
    
    while (!ipConnect()) {}
    while (!mqttConnect()) {}

    for (int i = 0; i < buffer_size; i++) {
        distanceBuffer[i] = measureDistance();
    }
}

void loop() {
    while (!ipConnect()) {}
    while (!mqttConnect()) {}
        Serial.println("wut");

    client.publish(status_topic, status_state_online);
    client.loop();

    distanceBuffer[cursor] = measureDistance();
    cursor = cursor + 1;
    cursor = cursor % buffer_size;

    if (average() > 13.0) {
        client.publish(state_topic, "closed");
    } else {
        client.publish(state_topic, "open");
    }

    delay(polling_rate_delay);
}
