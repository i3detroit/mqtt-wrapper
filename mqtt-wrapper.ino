/*
 * Example of how to use the i3-mqtt-client library.
 * Designed to hide the wifi and mqtt reconnect, as it doesn't matter.
 */
#include "mqtt-wrapper.h"


const char* ssid = "i3detroit-wpa";
const char* password = "i3detroit";
const char* mqtt_server = "10.13.0.22";
const int mqtt_port = 1883;

void callback(char* topic, byte* payload, unsigned int length, PubSubClient *client) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  client->publish("stat/i3/commons/lights/LightButton/status", "recieved msg");
}
void connectSuccess(PubSubClient* client) {
  Serial.println("win");
  //subscribe and shit here
  client->publish("stat/i3/commons/lights/LightButton/status", "online");
  client->subscribe("cmnd/i3/commons/lights/LightButton/something");
}
void setup() {
  Serial.begin(115200);
  setup_mqtt(callback, connectSuccess, ssid, password, mqtt_server, mqtt_port);
}
void connectedLoop() {
}
void loop() {
  loop_mqtt(connectedLoop);
}

