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

struct mqtt_wrapper_options* options;

char ip[16];


//Time since last mqtt connection attempt
uint32_t lastReconnectAttempt = 0;

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network

  if(options->debug_print) Serial.println();
  if(options->debug_print) Serial.print("Connecting to ");
  if(options->debug_print) Serial.println(options->ssid);


  WiFi.hostname(options->host_name);
  WiFi.begin(options->ssid, options->password);

  int i = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);

    if(options->debug_print) Serial.print(".");

  }

  // WiFi.macAddress(MAC_array);
  // for (int i = 0; i < sizeof(MAC_array); ++i){
  //   sprintf(MAC_char,"%s%02x:",MAC_char,MAC_array[i]);
  // }
  // MAC_char[strlen(MAC_char)-1] = '\0';

  randomSeed(micros());


  if(options->debug_print) Serial.println("");
  if(options->debug_print) Serial.println("WiFi connected");
  if(options->debug_print) Serial.println("IP address: ");
  if(options->debug_print) Serial.println(WiFi.localIP());
  //if(options->debug_print) Serial.println("MAC address: ");
  //if(options->debug_print) Serial.println(MAC_char);

}

boolean reconnect() {
  Serial.println("reconnect");
  if(WiFi.status() != WL_CONNECTED) {
    //Wifi is disconnected
    if(options->debug_print) Serial.println("WIFI BROKEN?");
  }
  int i = 0;
  Serial.println("reconnecting");
  while (!client.connected()) {

    if(options->debug_print) Serial.print("Attempting MQTT connection...");

    // Attempt to connect
    // client.connect(mqtt node name)
    Serial.println(options->host_name);
    Serial.println("actually calling connect");
    //if (client.connect(options->host_name, "tele/example/LWT", 0, true, "Offline")) {
    if (client.connect(options->host_name)) {

      if(options->debug_print) Serial.println("connected");


      WiFi.localIP().toString().toCharArray(ip, sizeof(ip));
      options->connectSuccess(&client, ip);
    } else {

      if(options->debug_print) Serial.print("failed, rc=");
      if(options->debug_print) Serial.print(client.state());
      if(options->debug_print) Serial.println(" try again in 5 seconds");

      // Wait 5 seconds before retrying
      delay(5000);
    }
  }

  return client.connected();
}

void internal_callback(char* topic, byte* payload, unsigned int length) {
  options->callback(topic, payload, length, &client);
}

//void setup_mqtt(void (*connectedLoop)(PubSubClient* client), void (*callback)(char* topic, uint8_t* payload, unsigned int length, PubSubClient* client), void (*connectSuccess)(PubSubClient* client, char* ip), const char* ssid, const char* password, const char* mqtt_server, int mqtt_port, const char* __host_name, bool debug_print) {
void setup_mqtt(struct mqtt_wrapper_options** newOptions) {
  options = *newOptions;
  if(options->debug_print == NULL) {
    options->debug_print = true;
  }

  setup_wifi();
  client.setServer(options->mqtt_server, options->mqtt_port);
  client.setCallback(internal_callback);

  ArduinoOTA.onStart([]() {

    if(options->debug_print) Serial.print("Start\n|");
    for(int i=0; i<80; ++i){
      if(options->debug_print) Serial.print(" ");
    }
    if(options->debug_print) Serial.print("|\n ");

  });
  ArduinoOTA.onEnd([]() {

    if(options->debug_print) Serial.println("\nEnd");

  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {

    //Static is fine becuase we restart after this. Or crash. Either way.
    static int curProg = 0;
    int x = map(progress,0,total,0,80);
    for(int i=curProg; i<x; ++i){
      if(options->debug_print) Serial.print("*");
    }

  });
  ArduinoOTA.onError([](ota_error_t error) {
    if(options->debug_print) Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) if(options->debug_print) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) if(options->debug_print) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) if(options->debug_print) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) if(options->debug_print) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) if(options->debug_print) Serial.println("End Failed");

  });
  ArduinoOTA.begin();
}
void loop_mqtt() {
  //Check if mqtt client is connected, if not try to reconnect to it(and wifi)
  //Fail in a non-blocking way
  Serial.println("LOOP");
  if (!client.connected()) {
    uint32_t now = millis();
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
    options->connectedLoop(&client);
  }//end mqtt client connected

  ArduinoOTA.handle();
}
