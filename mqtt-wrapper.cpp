/*
 * esp mqtt
 * Based on exmaple code from github.com/knolleary/pubsubclient
 * Copyright (c) 2016-2017 Mark Furland <mtfurlan@i3detroit.org>
 * MIT licence
*/

#include "mqtt-wrapper.h"
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ArduinoOTA.h>


ESP8266WiFiMulti wifiMulti;
WiFiClient espClient;
PubSubClient client(espClient);

struct mqtt_wrapper_options* options;
enum ConnState state;

uint32_t nextTelemetry;
uint32_t telemetryInterval;

char ip[16];
char mqtt_wrapper_buf[128];
char mqtt_wrapper_topic[64];
unsigned char macBin[6];
char mac[18];

//Time since last mqtt connection attempt
uint32_t lastReconnectAttempt = 0;

void info2() {
  sprintf(mqtt_wrapper_buf, "{\"Hostname\":\"%s\",\"IPaddress\":\"%s\",\"Mac\":\"%s\"}", options->host_name, ip, mac);
  sprintf(mqtt_wrapper_topic, "tele/%s/INFO2", options->fullTopic);
  client.publish(mqtt_wrapper_topic, mqtt_wrapper_buf);
}


boolean reconnect() {
  if(wifiMulti.run() != WL_CONNECTED) {
    if(state != WIFI_DISCONNECTED) {
      state = WIFI_DISCONNECTED;
      if(options->connectionEvent) {
        options->connectionEvent(&client, state, 0);
      }
    }
    //Wifi is disconnected
    if(options->debug_print) Serial.print(".");
  } else {
    //We have wifi
    if(state != F_MQTT_DISCONNECTED) {
      state = F_MQTT_DISCONNECTED;
      if(options->connectionEvent) {
        options->connectionEvent(&client, state, 0);
      }
    }
    if(options->debug_print) Serial.println("WiFi connected");
    if(options->debug_print) Serial.println("IP address: ");
    if(options->debug_print) Serial.println(WiFi.localIP());
    int i = 0;
    if (!client.connected()) {
      if(options->debug_print) Serial.print("Attempting MQTT connection...");

      // Attempt to connect
      // client.connect(mqtt node name)
      sprintf(mqtt_wrapper_buf, "tele/%s/LWT", options->fullTopic);
      if (client.connect(options->host_name, mqtt_wrapper_buf, 0, true, "Offline")) {
        if(state != F_MQTT_CONNECTED) {
          state = F_MQTT_CONNECTED;
          if(options->connectionEvent) {
            options->connectionEvent(&client, state, 0);
          }
        }

        if(options->debug_print) Serial.println("connected");


        WiFi.localIP().toString().toCharArray(ip, sizeof(ip));

        info2();
        sprintf(mqtt_wrapper_topic, "tele/%s/LWT", options->fullTopic);
        client.publish(mqtt_wrapper_topic, "Online", true);

        sprintf(mqtt_wrapper_topic, "cmnd/%s/+", options->fullTopic);
        client.subscribe(mqtt_wrapper_topic);

        options->connectSuccess(&client, ip);
      } else {
        if(state != F_MQTT_DISCONNECTED) {
          state = F_MQTT_DISCONNECTED;
          if(options->connectionEvent) {
            options->connectionEvent(&client, state, client.state());
          }
        }
        // https://pubsubclient.knolleary.net/api.html#state
        // int - the client state, which can take the following values (constants defined in PubSubClient.h):
        // -4 : MQTT_CONNECTION_TIMEOUT - the server didn't respond within the keepalive time
        // -3 : MQTT_CONNECTION_LOST - the network connection was broken
        // -2 : MQTT_CONNECT_FAILED - the network connection failed
        // -1 : MQTT_DISCONNECTED - the client is disconnected cleanly
        // 0 : MQTT_CONNECTED - the client is connected
        // 1 : MQTT_CONNECT_BAD_PROTOCOL - the server doesn't support the requested version of MQTT
        // 2 : MQTT_CONNECT_BAD_CLIENT_ID - the server rejected the client identifier
        // 3 : MQTT_CONNECT_UNAVAILABLE - the server was unable to accept the connection
        // 4 : MQTT_CONNECT_BAD_CREDENTIALS - the username/password were rejected
        // 5 : MQTT_CONNECT_UNAUTHORIZED - the client was not authorized to connect
        if(options->debug_print) Serial.print("failed, rc=");
        if(options->debug_print) Serial.println(client.state());

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

void setup_mqtt(struct mqtt_wrapper_options* newOptions) {
  options = newOptions;
  if(options->debug_print == NULL) {
    options->debug_print = true;
  }

  if(options->telemetryInterval) {
    telemetryInterval = options->telemetryInterval;
  } else {
    telemetryInterval = DEFAULT_TELEMETRY_INTERVAL;
  }
  nextTelemetry = telemetryInterval;

  if(options->debug_print) Serial.println();
  if(options->debug_print) Serial.print("Connecting to ");
  if(options->debug_print) Serial.println(options->ssid);

  WiFi.mode(WIFI_STA);

  WiFi.hostname(options->host_name);
  wifiMulti.addAP(options->ssid, options->password);
  wifiMulti.run();

  WiFi.macAddress(macBin);
  for (int i = 0; i < sizeof(macBin); ++i){
    sprintf(mac + strlen(mac),"%02X:",macBin[i]);
  }
  mac[strlen(mac)-1] = '\0';
  if(options->debug_print) Serial.print("MAC: ");
  if(options->debug_print) Serial.println(mac);

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
  //TODO: count failures, do something like reboot or host network?
  if (!client.connected()) {
    uint32_t now = millis();
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      // Attempt to reconnect
      if (reconnect()) {
        lastReconnectAttempt = 0;
      }
    }
  } else {
    // mqtt client connected
    client.loop();//Look for messages and whatnot...
    options->connectedLoop(&client);
    if(options->telemetry) {
      if( (long)( millis() - nextTelemetry ) >= 0) {
        nextTelemetry = millis() + telemetryInterval;
        options->telemetry(&client);
      }
    }
  }//end mqtt client connected

  ArduinoOTA.handle();
}
