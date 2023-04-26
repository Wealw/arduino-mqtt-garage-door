# Arduino-Mqtt-garage-door

This is a litle Arduino project to control a garage door with a relay and an ultrasound proximity sensor via MQTT.

## Electronic 

[WIP]

## Secret.h file structure

```c
#ifndef ARDUINO_MQTT_GARAGE_DOOR_SECRET_H
#define ARDUINO_MQTT_GARAGE_DOOR_SECRET_H

// Pin configuration
#define relay 5
#define trigPin 6
#define echoPin 7

// IP configuration
byte mac[] = { 0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02 };

// MQTT configuration
#define username "USERNAME"
#define password "PASSWORD"
#define port 1883
const IPAddress broker(10,0,30,2);

// Topic configuration
#define state_topic "state/garage_door_example"
#define command_topic "command/garage_door_example"
#define status_topic "telemetry/garage_door_example"

// Topic value configuration
#define status_state_online "online"
#define max_open_time 18000
#define polling_rate_delay 100

#define buffer_size 7

#endif //ARDUINO_MQTT_GARAGE_DOOR_SECRET_H

```
