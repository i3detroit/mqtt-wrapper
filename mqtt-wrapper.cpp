/*
 * esp mqtt
 * Based on exmaple code from github.com/knolleary/pubsubclient
 * Copyright (c) 2016-2017 Mark Furland <mtfurlan@i3detroit.org>
 * MIT licence
*/

#include "mqtt-wrapper.h"
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>


WiFiClient espClient;
PubSubClient client(espClient);
void (*_connectSuccess)(PubSubClient* client);
void (*_connectedLoop)(PubSubClient* client);
void (*_callback)(char* topic, uint8_t* payload, unsigned int length, PubSubClient* client);

char _hostname[120];

bool _debug_print;


//Time since last mqtt connection attempt
long lastReconnectAttempt = 0;
//Time since last mqtt status
long lastMsg = 0;
//Buffers for mqtt data
char topic_buf[120];
char data_buf[120];

void setup_wifi(const char* ssid, const char* password) {

  delay(10);
  // We start by connecting to a WiFi network

  if(_debug_print) Serial.println();
  if(_debug_print) Serial.print("Connecting to ");
  if(_debug_print) Serial.println(ssid);


  WiFi.hostname(_hostname);
  WiFi.begin(ssid, password);

  int i = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);

    if(_debug_print) Serial.print(".");

  }

  // WiFi.macAddress(MAC_array);
  // for (int i = 0; i < sizeof(MAC_array); ++i){
  //   sprintf(MAC_char,"%s%02x:",MAC_char,MAC_array[i]);
  // }
  // MAC_char[strlen(MAC_char)-1] = '\0';

  randomSeed(micros());


  if(_debug_print) Serial.println("");
  if(_debug_print) Serial.println("WiFi connected");
  if(_debug_print) Serial.println("IP address: ");
  if(_debug_print) Serial.println(WiFi.localIP());
  //if(_debug_print) Serial.println("MAC address: ");
  //if(_debug_print) Serial.println(MAC_char);

}

boolean reconnect() {
  if(WiFi.status() != WL_CONNECTED) {
    //Wifi is disconnected
    if(_debug_print) Serial.println("WIFI BROKEN?");
  }
  int i = 0;
  while (!client.connected()) {

    if(_debug_print) Serial.print("Attempting MQTT connection...");

    // Attempt to connect
    // client.connect(mqtt node name)
    if (client.connect(_hostname)) {

      if(_debug_print) Serial.println("connected");

      _connectSuccess(&client);
    } else {

      if(_debug_print) Serial.print("failed, rc=");
      if(_debug_print) Serial.print(client.state());
      if(_debug_print) Serial.println(" try again in 5 seconds");

      // Wait 5 seconds before retrying
      delay(5000);
    }
  }

  return client.connected();
}

void internal_callback(char* topic, byte* payload, unsigned int length) {
  _callback(topic, payload, length, &client);
}

void setup_mqtt(void (*connectedLoop)(PubSubClient* client), void (*callback)(char* topic, uint8_t* payload, unsigned int length, PubSubClient* client), void (*connectSuccess)(PubSubClient* client), const char* ssid, const char* password, const char* mqtt_server, int mqtt_port, const char* __hostname, bool debug_print) {
  _connectSuccess = connectSuccess;
  _callback = callback;
  _connectedLoop = connectedLoop;
  _debug_print = debug_print;

  int i;
  for(i=0; i<strlen(__hostname); ++i) {
    _hostname[i] = __hostname[i];
  }
  _hostname[i] = '\0';

  setup_wifi(ssid, password);
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(internal_callback);

  ArduinoOTA.onStart([]() {

    if(_debug_print) Serial.print("Start\n|");
    for(int i=0; i<80; ++i){
      if(_debug_print) Serial.print(" ");
    }
    if(_debug_print) Serial.print("|\n ");

  });
  ArduinoOTA.onEnd([]() {

    if(_debug_print) Serial.println("\nEnd");

  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {

    //Static is fine becuase we restart after this. Or crash. Either way.
    static int curProg = 0;
    int x = map(progress,0,total,0,80);
    for(int i=curProg; i<x; ++i){
      if(_debug_print) Serial.print("*");
    }

  });
  ArduinoOTA.onError([](ota_error_t error) {
    if(_debug_print) Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) if(_debug_print) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) if(_debug_print) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) if(_debug_print) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) if(_debug_print) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) if(_debug_print) Serial.println("End Failed");

  });
  ArduinoOTA.begin();
}
void loop_mqtt() {
  //Check if mqtt client is connected, if not try to reconnect to it(and wifi)
  //Fail in a non-blocking way
  if (!client.connected()) {
    long now = millis();
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      // Attempt to reconnect
      if (reconnect()) {
        lastReconnectAttempt = 0;
      }
    }
  }else{
    // mqtt client connected
    client.loop();//Look for messages and whatnot...
    _connectedLoop(&client);
  }//end mqtt client connected

  ArduinoOTA.handle();
}
