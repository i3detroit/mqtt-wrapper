/*
 * esp mqtt
 * Based on exmaple code from github.com/knolleary/pubsubclient
 * Copyright (c) 2016-2017 Mark Furland <mtfurlan@i3detroit.org>
 * MIT licence
*/

/*
 * TODO: mqtt client name
 */

#ifndef _MQTT_WRAPPER_H_
#define _MQTT_WRAPPER_H_


#include <PubSubClient.h>

extern void setup_mqtt(void (*connectedLoop)(PubSubClient* client), void (*callback)(char* topic, uint8_t* payload, unsigned int length, PubSubClient* client), void (*connectSuccess)(PubSubClient* client), const char* ssid, const char* password, const char* mqtt_server, int mqtt_port, const char* __hostname);
// probably something like void callback(char* topic, byte* payload, unsigned int length);
extern void loop_mqtt();

#endif /* _MQTT_WRAPPER_H_ */
