/*
 * Example of how to use the i3-mqtt-client library.
 * Designed to hide the wifi and mqtt reconnect, as it doesn't matter.
 */
#include "mqtt-wrapper.h"

#include "SSD1306.h" // alias for `#include "SSD1306Wire.h"`


const char* host_name = "display-example";
const char* ssid = "i3detroit-wpa";
const char* password = "i3detroit";
const char* mqtt_server = "10.13.0.22";
const int mqtt_port = 1883;

char buf[1024];

SSD1306  display(0x3c, D3, D5);

void callback(char* topic, byte* payload, unsigned int length, PubSubClient *client) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    //buf[i] = (char)payload[i];
  }
  Serial.println();
  if (strcmp(topic, "cmnd/i3/commons/oled/display") == 0) {

    int j = 0;
    for(; j<length && j<sizeof(buf)/sizeof(char); ++j) {
      buf[j] = (char)payload[j];
      Serial.println((char)payload[j]);
    }
    buf[j] = '\0';
    Serial.print("got '");
    Serial.print(buf);
    Serial.println("'");


    display.clear();
    display.drawStringMaxWidth(0, 0, 128, String(buf));
    display.display();
    client->publish("stat/i3/commons/oled/status", "displayed msg");
  } else {
    client->publish("stat/i3/commons/oled/status", "unknown command");
  }
}
void connectSuccess(PubSubClient* client, char* ip) {
  Serial.println("win");
  //subscribe and shit here
  client->publish("stat/i3/commons/oled/status", ip);
  client->subscribe("cmnd/i3/commons/oled/display");
}

void setup() {
  Serial.begin(115200);
  setup_mqtt(connectedLoop, callback, connectSuccess, ssid, password, mqtt_server, mqtt_port, host_name);




  display.init();
  display.flipScreenVertically();
  display.setContrast(255);

  // Align text vertical/horizontal center
  display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
  display.setFont(ArialMT_Plain_10);
  display.drawString(DISPLAY_WIDTH/2, DISPLAY_HEIGHT/2, "Hello World");
  display.display();
  //Future ones left
  display.setTextAlignment(TEXT_ALIGN_LEFT);
}
void connectedLoop(PubSubClient* client) {
}
void loop() {
  loop_mqtt();
}

