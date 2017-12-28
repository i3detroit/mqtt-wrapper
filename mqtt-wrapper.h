/*
 * esp mqtt
 * Based on exmaple code from github.com/knolleary/pubsubclient
 * Copyright (c) 2016-2017 Mark Furland <mtfurlan@i3detroit.org>
 * MIT licence
*/

#ifndef _MQTT_WRAPPER_H_
#define _MQTT_WRAPPER_H_


//Does nothing? Has to be modified in source?
#define MQTT_MAX_PACKET_SIZE 512
#include <PubSubClient.h>


struct mqtt_wrapper_options {
  void (*connectedLoop)(PubSubClient* client);
  void (*callback)(char* topic, uint8_t* payload, unsigned int length, PubSubClient* client);
  void (*connectSuccess)(PubSubClient* client, char* ip);
  const char* ssid;
  const char* password;
  const char* mqtt_server;
  int mqtt_port;
  const char* host_name;
  const char* fullTopic;
  bool debug_print;
};


extern void setup_mqtt(struct mqtt_wrapper_options* options);
extern void loop_mqtt();

#endif /* _MQTT_WRAPPER_H_ */
