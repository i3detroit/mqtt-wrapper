/*
 * Example of how to use the i3-mqtt-client library.
 * Designed to hide the wifi and mqtt reconnect, as it doesn't matter.
 */
#include "mqtt-wrapper.h"

const char* host_name = "basic-example";
const char* ssid = "i3detroit-wpa";
const char* password = "i3detroit";
const char* mqtt_server = "10.13.0.22";
const int mqtt_port = 1883;
const char* fullTopic = "test";

struct mqtt_wrapper_options some_options;
char buf[1024];

void callback(char* topic, byte* payload, unsigned int length, PubSubClient *client) {
    Serial.print("user given [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i = 0; i < length; i++) {
      Serial.print((char)payload[i]);
    }
    Serial.println();
  client->publish("stat/example/status", "recieved msg");
}
void connectSuccess(PubSubClient* client, char* ip) {
  Serial.println("win");
  //subscribe and shit here
  //Are already subscribed to cmnd/fullTopic/+
  //And will recieve callback topics as everything after the trailing slash
  //client->subscribe("cmnd/example/doStuff");
}
void setup() {
  Serial.begin(115200);
  some_options.connectedLoop = connectedLoop;
  some_options.callback = callback;
  some_options.connectSuccess = connectSuccess;
  some_options.ssid = ssid;
  some_options.password = password;
  some_options.mqtt_server = mqtt_server;
  some_options.mqtt_port = mqtt_port;
  some_options.host_name = host_name;
  some_options.fullTopic = fullTopic;
  setup_mqtt(&some_options);
  Serial.println("Setup done");
}
void connectedLoop(PubSubClient* client) {
}
void loop() {
  loop_mqtt();
}

