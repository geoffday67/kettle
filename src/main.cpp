#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoHttpClient.h>

#define BUTTON_PIN_BITMASK 0x200000000 // 2^33 in hex
#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */

#define KETTLE_HOST     "192.168.68.106"
#define KETTLE_PORT     1968
#define KETTLE_URI      "/interface"

#define RED_LED     25
#define GREEN_LED   26
#define BLUE_LED    27

WiFiClient wifi;

void showLED(int led) {
  switch(led) {
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

    WiFi.hostname("Kettle");
    WiFi.begin("Wario", "mansion1");
    Serial.print("Connecting to Wario ");

    start = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start > 20000) {
            Serial.println();
            Serial.println("Timed out connecting to access point");
            goto exit;
        }
        delay(100);
        Serial.print(".");
    }
    Serial.println();
    Serial.print("Connected, IP address: ");
    Serial.println(WiFi.localIP());

    result = true;

exit:
    return result;
}

void disconnectWiFi() {
  WiFi.mode(WIFI_OFF);
  Serial.println("WiFi disconnected");
}

bool kettleOn() {
  bool result = false;
  HttpClient client = HttpClient(wifi, KETTLE_HOST, KETTLE_PORT);
  char body[] = "operation=433&channel=3&command=on";
  int code;

  if (!connectWiFi()) {
    Serial.println("Error connecting WiFi");
    goto exit;
  }

  Serial.printf ("Sending: %s\n", body);

  client.beginRequest();
  client.post(KETTLE_URI);
  client.sendHeader("Content-Type", "application/x-www-form-urlencoded");
  client.sendHeader("Content-Length", strlen(body));
  client.sendBasicAuth("wario", "mansion1");
  client.beginBody();
  client.print(body);
  client.endRequest();

  code = client.responseStatusCode();

  if (code != 200) {
    Serial.printf ("Error response %d\n", code);
    goto exit;
  }

  Serial.printf ("Received code %d: %s\n", code, client.responseBody().c_str());

  result = true;

exit:
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

void setup(){
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

void loop(){
  //This is not going to be called
}
