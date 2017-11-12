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
const char* fullTopic = "example";

char buf[1024];

void callback(char* topic, byte* payload, unsigned int length, PubSubClient *client) {
  Serial.print("Message arrived [");
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
  sprintf(buf, "{\"Hostname\":\"%s\", \"IPaddress\":\"%s\"}", host_name, ip);
  client->publish("tele/example/INFO2", buf);
  client->publish("tele/example/LWT", "Online");
  client->subscribe("cmnd/example/doStuff");
}
void setup() {
  Serial.begin(115200);
  struct mqtt_wrapper_options options;
  options.connectedLoop = connectedLoop;
  options.callback = callback;
  options.connectSuccess = connectSuccess;
  options.ssid = ssid;
  options.password = password;
  options.mqtt_server = mqtt_server;
  options.mqtt_port = mqtt_port;
  options.host_name = host_name;
  options.fullTopic = fullTopic;
  struct mqtt_wrapper_options* tmp = &options;
  setup_mqtt(&tmp);
  Serial.println("Setup done");
}
void connectedLoop(PubSubClient* client) {
}
void loop() {
  loop_mqtt();
}

