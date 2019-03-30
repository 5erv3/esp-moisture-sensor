#include <WiFi.h>
#include <PubSubClient.h>
#include "config.h"

// uncomment logging when not needed
#define LOGGING  1

#define TIME_SLEEPTIME_S          3600UL
#define TIME_FACTOR_US_TO_S       (1000UL*1000UL)
#define WIFI_MAX_CONNECT_TIME_SEC 30
#define MQTT_RECONNECT_ATTEMPTS   3

WiFiClient espClient;
PubSubClient client(espClient);

char buf[100];
int reconnect_counter = 0;

void LOG(const char* logstring, bool newline = true){
  #if LOGGING
  if (newline){
    Serial.println(logstring);
  } else {      
    Serial.print(logstring);
  }
  #endif
}

void setup() {
  pinMode(moistureSensPin, INPUT);
#if LOGGING
  Serial.begin(115200);
#endif
  
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
}

void deepsleep(int sleeptime = TIME_SLEEPTIME_S){
  sprintf(buf, "Going to deepsleep for %d seconds...", sleeptime);
  LOG(buf);
  esp_deep_sleep(TIME_FACTOR_US_TO_S * sleeptime);
  esp_deep_sleep_start();
}

void setup_wifi() {
  delay(10);

  sprintf(buf, "Connecting to %s...", wifi_ssid);
  LOG(buf);

  WiFi.begin(wifi_ssid, wifi_password);

  reconnect_counter = 0;

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    reconnect_counter ++;
    if (reconnect_counter >= WIFI_MAX_CONNECT_TIME_SEC){
      LOG("ERROR: WiFi connection counter expired");
      deepsleep();
    }
  }  
  sprintf(buf, "WiFi connected, IP address: ");
  LOG(buf, false);
  Serial.print(WiFi.localIP());
  LOG("");
}

void reconnect() {
  reconnect_counter = 0;
  while (!client.connected()) {
    LOG("Attempting MQTT connection...");
    if (client.connect("ESP8266Client")) {
      LOG("connected", true);
    } else {
      reconnect_counter++;
      if (reconnect_counter >= MQTT_RECONNECT_ATTEMPTS){
        LOG("ERROR: too many mqtt attempts");
        deepsleep();
      }      
      sprintf(buf, "connection failed, rc %d", client.state());
      LOG(buf);      
      LOG(" try again in 5 seconds", true);
      delay(5000);
    }
  }
}

int getMoisture(){
  return analogRead(moistureSensPin);
}

void loop() {
  
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  sprintf(buf, "%d", getMoisture());

  client.publish(mqtt_topic, buf);
  deepsleep();
}
