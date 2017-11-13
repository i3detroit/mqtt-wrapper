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
char mqtt_wrapper_buf[64];
char mqtt_wrapper_topic[64];

unsigned long nextMQTTAttempt = 0UL;
unsigned long mqttAttemptInterval = 5000UL;

//Time since last mqtt connection attempt
uint32_t lastReconnectAttempt = 0;

void info2() {
  // WiFi.macAddress(MAC_array);
  // for (int i = 0; i < sizeof(MAC_array); ++i){
  //   sprintf(MAC_char,"%s%02x:",MAC_char,MAC_array[i]);
  // }
  // MAC_char[strlen(MAC_char)-1] = '\0';
  sprintf(mqtt_wrapper_buf, "{\"Hostname\":\"%s\", \"IPaddress\":\"%s\"}", options->host_name, ip);
  sprintf(mqtt_wrapper_topic, "tele/%s/INFO2", options->fullTopic);
  client.publish(mqtt_wrapper_topic, mqtt_wrapper_buf);
}


void setup_wifi() {




  WiFi.hostname(options->host_name);
  WiFi.begin(options->ssid, options->password);

  // while (WiFi.status() != WL_CONNECTED) {
  //   delay(500);

  //   if(options->debug_print) Serial.print(".");

  // }



  //if(options->debug_print) Serial.println("MAC address: ");
  //if(options->debug_print) Serial.println(MAC_char);

}

boolean reconnect() {
  if(WiFi.status() != WL_CONNECTED) {
    //Wifi is disconnected
    if(options->debug_print) Serial.print(".");
  } else {
    //We have wifi
    if(options->debug_print) Serial.println("WiFi connected");
    if(options->debug_print) Serial.println("IP address: ");
    if(options->debug_print) Serial.println(WiFi.localIP());
    //We have wifi
    int i = 0;
    if (!client.connected() && (long)( millis() - nextMQTTAttempt ) >= 0) {
      nextMQTTAttempt = millis() + mqttAttemptInterval;
      if(options->debug_print) Serial.print("Attempting MQTT connection...");

      // Attempt to connect
      // client.connect(mqtt node name)
      sprintf(mqtt_wrapper_buf, "tele/%s/LWT", options->fullTopic);
      if (client.connect(options->host_name, mqtt_wrapper_buf, 0, true, "Offline")) {

        if(options->debug_print) Serial.println("connected");


        WiFi.localIP().toString().toCharArray(ip, sizeof(ip));

        info2();
        sprintf(mqtt_wrapper_topic, "tele/%s/LWT", options->fullTopic);
        client.publish(mqtt_wrapper_topic, "Online");

        sprintf(mqtt_wrapper_topic, "cmnd/%s/+", options->fullTopic);
        client.subscribe(mqtt_wrapper_topic);

        options->connectSuccess(&client, ip);
      } else {

        if(options->debug_print) Serial.print("failed, rc=");
        if(options->debug_print) Serial.print(client.state());

      }
    }
  }
  return client.connected();
}

void internal_callback(char* topic, byte* payload, unsigned int length) {
  if(options->debug_print) {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i = 0; i < length; i++) {
      Serial.print((char)payload[i]);
    }
    Serial.println();
  }
  // if it is addressed as a command to the fulltopic, parse it
  sprintf(mqtt_wrapper_buf, "cmnd/%s/", options->fullTopic);
  if(strncmp(mqtt_wrapper_buf, topic, strlen(mqtt_wrapper_buf)) == 0) {
    //Find last bit of topic
    int i = strlen(topic);
    while(topic[i] != '/') {
      // to lower
      // https://stackoverflow.com/a/2661917/2423187
      topic[i] = (topic[i] > 0x40 && topic[i] < 0x5b) ? topic[i]|0x60 : topic[i];
      --i;
    }
    topic += i + 1;
    //Handle some commands, return from function if handled
    if(strcmp(topic, "restart") == 0) {
      if(payload[0] == '1' && length == 1) {
        if(options->debug_print) Serial.print("RESTART!");
        sprintf(mqtt_wrapper_topic, "stat/%s/RESULT", options->fullTopic);
        client.publish(mqtt_wrapper_topic, "{\"Restart\":\"Restarting\"}");
        ESP.restart();
      } else {
        if(options->debug_print) Serial.print("try better options?");
        sprintf(mqtt_wrapper_topic, "stat/%s/RESULT", options->fullTopic);
        client.publish(mqtt_wrapper_topic, "{\"Restart\":\"1 to restart\"}");
      }
      return;
    } else if (strcmp(topic, "status") == 0){
      //TODO: This does not match tasmota format. I am not sure I care.
      if(options->debug_print) Serial.print("INFO!");
      info2();
      return;
    }
  }
  //Might have been parsed, but was not handled by function
  options->callback(topic, payload, length, &client);
}

//void setup_mqtt(void (*connectedLoop)(PubSubClient* client), void (*callback)(char* topic, uint8_t* payload, unsigned int length, PubSubClient* client), void (*connectSuccess)(PubSubClient* client, char* ip), const char* ssid, const char* password, const char* mqtt_server, int mqtt_port, const char* __host_name, bool debug_print) {
void setup_mqtt(struct mqtt_wrapper_options* newOptions) {
  options = newOptions;
  if(options->debug_print == NULL) {
    options->debug_print = true;
  }

  setup_wifi();
  if(options->debug_print) Serial.println();
  if(options->debug_print) Serial.print("Connecting to ");
  if(options->debug_print) Serial.println(options->ssid);

  WiFi.hostname(options->host_name);
  WiFi.begin(options->ssid, options->password);


  randomSeed(micros());
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
