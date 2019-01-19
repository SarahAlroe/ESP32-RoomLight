#include <WiFi.h>
#include <HTTPClient.h>
#include <ESPmDNS.h>
#include <ESP32Encoder.h>
#include <AceButton.h>
#include "Config.h"

using namespace ace_button;

const char *ssid = WIFI_SSID;
const char *password = WIFI_PASS;
const char *masterName = HOSTNAME;
const int rEncoderPins[] = {0, 16, 23};
const char *controlDir = "/extCont";
IPAddress masterAddress = IPAddress();
ESP32Encoder encoder;
HTTPClient http;
AceButton button(rEncoderPins[2]);
void buttonEvent(AceButton*, uint8_t, uint8_t);

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.begin(115200);

  pinMode(rEncoderPins[0], INPUT_PULLUP);
  pinMode(rEncoderPins[1], INPUT_PULLUP);
  
  encoder.setCount(0);
  encoder.attachHalfQuad(rEncoderPins[0], rEncoderPins[1]);

  pinMode(rEncoderPins[2], INPUT_PULLUP);
  button.setEventHandler(buttonEvent);
  ButtonConfig* buttonConfig = button.getButtonConfig();
  buttonConfig->setEventHandler(buttonEvent);
  buttonConfig->setFeature(ButtonConfig::kFeatureClick);
  buttonConfig->setFeature(ButtonConfig::kFeatureDoubleClick);
  buttonConfig->setFeature(ButtonConfig::kFeatureLongPress);
  buttonConfig->setFeature(ButtonConfig::kFeatureSuppressAll);
  //buttonConfig->setFeature(ButtonConfig::kFeatureSuppressClickBeforeDoubleClick);

  Serial.println("Connecting to wifi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  WiFi.setHostname("RLC-UID");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  MDNS.begin("ESP32SMLUID");
  Serial.println("Getting master address");
  while (masterAddress == IPAddress()) {
    Serial.print(".");
    masterAddress = MDNS.queryHost(masterName);
  }
  Serial.println("Found master at: " + masterAddress.toString());
  http.setReuse(true);
  digitalWrite(LED_BUILTIN, LOW);
}

void loop() {
  button.check();
  if (encoder.getCount() != 0) {
    sendRotaryEvent(encoder.getCount());
  }else{
  delay(50);
  }
}

void buttonEvent(AceButton* /* button */, uint8_t eventType,
                 uint8_t /* buttonState */) {
  switch (eventType) {
    case AceButton::kEventClicked:
      sendClickEvent(1);
      break;
    case AceButton::kEventDoubleClicked:
      sendClickEvent(2);
      break;
    case AceButton::kEventLongPressed:
      sendClickEvent(3);
      break;
  }
}

void sendClickEvent(int count) {
  http.begin("http://" + masterAddress.toString() + controlDir);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  int httpCode = http.POST("click=" + String(count));
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    Serial.println(payload);
  } else {
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    String payload = http.getString();
    Serial.println(payload);
  }
  http.end();
}

void sendRotaryEvent(int rotationCount) {
  http.begin("http://" + masterAddress.toString() + controlDir);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  String postMessage;
  if (rotationCount > 0) {
    postMessage = "increment=" + String(rotationCount);
  } else {
    postMessage = "decrement=" + String(abs(rotationCount));
  }
  encoder.setCount(0);
  int httpCode = http.POST(postMessage);
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    Serial.println(payload);
  } else {
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    String payload = http.getString();
    Serial.println(payload);
  }
  http.end();
}
