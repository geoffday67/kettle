#include <Arduino.h>
#include <WiFi.h>

#include "mqtt.h"

#define BUTTON_PIN_BITMASK 0x200000000  // 2^33 in hex
#define uS_TO_S_FACTOR 1000000          /* Conversion factor for micro seconds to seconds */

#define MQTT_SERVER "192.168.68.106"
#define MQTT_PORT 1883
#define MQTT_CLIENT "kettle-remote"
#define MQTT_TOPIC "switches/kettle"
#define MQTT_ON_MESSAGE "on"

#define RED_LED 25
#define GREEN_LED 26
#define BLUE_LED 27

WiFiClient wifi;

void showLED(int led) {
  switch (led) {
    case RED_LED:
      digitalWrite(RED_LED, LOW);
      digitalWrite(GREEN_LED, HIGH);
      digitalWrite(BLUE_LED, HIGH);
      break;
    case GREEN_LED:
      digitalWrite(RED_LED, HIGH);
      digitalWrite(GREEN_LED, LOW);
      digitalWrite(BLUE_LED, HIGH);
      break;
    case BLUE_LED:
      digitalWrite(RED_LED, HIGH);
      digitalWrite(GREEN_LED, HIGH);
      digitalWrite(BLUE_LED, LOW);
      break;
  }
}

bool connectWiFi() {
  bool result = false;
  unsigned long start;
  int tries;

  for (tries = 0; tries < 3; tries++) {
    WiFi.hostname("Kettle");
    WiFi.begin("Wario", "mansion1");
    Serial.print("Connecting to Wario ");

    start = millis();
    while (WiFi.status() != WL_CONNECTED) {
      if (millis() - start > 5000) {
        Serial.println();
        Serial.println("Timed out connecting to access point");
        break;
      }
      delay(100);
      Serial.print(".");
    }
    if (WiFi.status() == WL_CONNECTED) {
      break;
    }
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.printf("Failed to connect to WiFi after %d tries\n", tries);
    goto exit;
  }

  Serial.println();
  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());

  result = true;

exit:
  return result;
}

void disconnectWiFi() {
  wifi.flush();
  WiFi.mode(WIFI_OFF);
  Serial.println("WiFi disconnected");
}

bool kettleOn() {
  MQTT mqtt(wifi);
  bool result = false;

  if (!connectWiFi()) {
    Serial.println("Error connecting WiFi");
    goto exit;
  }

  if (!mqtt.connect(MQTT_SERVER, MQTT_PORT, MQTT_CLIENT)) {
    Serial.println("Error connecting MQTT");
    goto exit;
  }

  if (!mqtt.publish(MQTT_TOPIC, MQTT_ON_MESSAGE)) {
    Serial.println("Error publishing to MQTT");
    goto exit;
  }

  result = true;

exit:
  mqtt.disconnect();
  disconnectWiFi();
  return result;
}

void handleButtonWakeup() {
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);

  showLED(BLUE_LED);

  if (kettleOn()) {
    showLED(GREEN_LED);
  } else {
    showLED(RED_LED);
  }
  delay(2000);
  esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK, ESP_EXT1_WAKEUP_ANY_HIGH);
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting");

  switch (esp_sleep_get_wakeup_cause()) {
    case ESP_SLEEP_WAKEUP_EXT1:
      handleButtonWakeup();
      break;
    default:
      esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK, ESP_EXT1_WAKEUP_ANY_HIGH);
      break;
  }

  Serial.println("Sleeping");
  Serial.flush();
  esp_deep_sleep_start();
}

void loop() {
  // This is not going to be called
}
