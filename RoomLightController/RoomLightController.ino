//Imports
#include <WiFi.h>
#include <WiFiClient.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <NeoPixelBus.h>
#include <NeoPixelAnimator.h>
#include <ESPAsyncWebServer.h>
#include "time.h"
#include "State.h"
#include "Type.h"
#include <Preferences.h>

//User configurable constants
const uint16_t PixelCount = 60 * 4;
const uint16_t PixelPin = 12;

const char *ssid = "WIFI SSID HERE";
const char *password = "WIFI PASSWORD HERE";

const int stateCount = 5;
const int typeCount = 4;

const char *mdnsName = "RLC";
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;

//LED wavelengths as according to WS2812 spec sheet.
const int rwl = 625; //Red
const int gwl = 523; //Green
const int bwl = 470; //Blue

//Init
Preferences preferences;
AsyncWebServer server(80);
NeoPixelBus<NeoRgbwFeature, Neo800KbpsMethod> strip(PixelCount, PixelPin);
NeoPixelAnimator animations(2);
NeoGamma<NeoGammaTableMethod> colorGamma;

//Global vars
Type types[typeCount];
State states[stateCount];

int currentState = 0;
int targetState = 0;
float targetBrightness = 0;
float currentBrightness = 0;
float outsetBrightness = currentBrightness;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.begin(115200);
  Serial.println("Beginning setup:");

  Serial.print("WIFI: ");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  WiFi.setHostname(mdnsName);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("ok");

  Serial.print("OTA: ");
  setupOTA();
  Serial.println("ok");

  Serial.print("NTP: ");
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.println("ok");

  Serial.print("MDNS: ");
  MDNS.begin(mdnsName);
  MDNS.addService("http", "tcp", 80);
  MDNS.setInstanceName("Esp32 SmartRoomLight Controller");
  Serial.println("ok");

  Serial.print("Setup types: ");
  setupTypes();
  Serial.println("ok");
  Serial.print("Setup states: ");
  setupStates();
  Serial.println("ok");

  Serial.print("Setup webserver: ");
  server.on("/", HTTP_ANY, handleRoot);
  server.on("/extCont", HTTP_ANY, handleExternal);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("ok");

  Serial.print("LED Strip: ");
  strip.Begin();
  strip.Show();
  setBrightness(255 / 2);
  setState(1);
  Serial.println("ok");
  Serial.println("Setup complete");
  digitalWrite(LED_BUILTIN, LOW);
}

void loop() {
  ArduinoOTA.handle();
  animations.UpdateAnimations();
  strip.Show();
}

//Setup extractions
void setupOTA() {
  ArduinoOTA
  .onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  })
  .onEnd([]() {
    Serial.println("\nEnd");
  })
  .onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  })
  .onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.setHostname(mdnsName);
  ArduinoOTA.begin();
}

void setupTypes() {
  State &state = getNState();
  types[0] = Type(StaticColorAnimUpdate, "Static color");
  types[1] = Type(ColorTemperatureAnimUpdate, "Dynamic color temperature");
  types[2] = Type(StaticPatternAnimUpdate, "Static pattern");
  types[3] = Type(RainbowAnimUpdate, "Rainbow fade");

  types[0].setSettingsHTML("<input type='hidden' name='ts' value='sc'/>"
                           "<label>Color:</label><br/>"
                           "<label>R: </label><input type='number' name='0R' min='0' max='255' value='" + String(state.getColor(0).G) + "'/><br/>"
                           "<label>G: </label><input type='number' name='0G' min='0' max='255' value='" + String(state.getColor(0).R) + "'/><br/>"
                           "<label>B: </label><input type='number' name='0B' min='0' max='255' value='" + String(state.getColor(0).B) + "'/><br/>"
                           "<label>W: </label><input type='number' name='0W' min='0' max='255' value='" + String(state.getColor(0).W) + "'/><br/>");

  types[1].setSettingsHTML("<input type='hidden' name='ts' value='dct'/>"
                           "<label>Day temperature: </label>"
                           "<input type='range' name='dt' min='0' max='" + String(255 * 3) + "' value='" + state.getDayTemp() + "'/><br />"
                           "<label>Night temperature: </label>"
                           "<input type='range' name='nt' min='0' max='" + String(255 * 3) + "' value='" + state.getNightTemp() + "'/><br />"
                           "<label>Day time: </label>"
                           "<input type='time' name='det' value='" + getTimeString(state.getDayTime()) + "'/>"
                           "<label>Night time: </label>"
                           "<input type='time' name='nit' value='" + getTimeString(state.getNightTime()) + "'/><br />"
                           "<label>Trans. time (min): </label>"
                           "<input type='number' name='tti' value='" + state.getTransTime() + "'/><br/>");

  String settings = "<input type='hidden' name='ts' value='sp'/><div class='row'>";
  for (int i = 0; i < state.colorCount; i++) {
    settings += "<div class='col-md'><label>Color" + String(i) + ":</label><br/>"
                "<label>R: </label><input type='number' name='" + String(i) + "R' min='0' max='255' value='" + String(state.getColor(i).G) + "'/><br/>"
                "<label>G: </label><input type='number' name='" + String(i) + "G' min='0' max='255' value='" + String(state.getColor(i).R) + "'/><br/>"
                "<label>B: </label><input type='number' name='" + String(i) + "B' min='0' max='255' value='" + String(state.getColor(i).B) + "'/><br/>"
                "<label>W: </label><input type='number' name='" + String(i) + "W' min='0' max='255' value='" + String(state.getColor(i).W) + "'/><br/></div>";
  }
  settings += "</div><br/>";
  types[2].setSettingsHTML(settings);

  types[3].setSettingsHTML("<input type='hidden' name='ts' value='rf'/>"
                           "<label>Animation slowness: </label><br/>"
                           "<input type='number' name='rft' value='" + String(state.getAnimationDelay()) + "'/><br/>");

}

//Web handles
void handleNotFound(AsyncWebServerRequest *request) {
  String message = "<html>"
                   "<head><title>404 - Room Light Control</title><link rel='stylesheet' href='https://stackpath.bootstrapcdn.com/bootstrap/4.1.3/css/bootstrap.min.css'></head>"
                   "<body><div class='container'>"
                   "<h1>File Not Found</h1>";
  message += "URI: ";
  message += request->url();
  message += "<br/>Method: ";
  message += (request->method() == HTTP_GET) ? "GET" : "POST";
  message += "<br/>Arguments: ";
  message += request->params();
  message += "<br/>";

  for (uint8_t i = 0; i < request->params(); i++) {
    AsyncWebParameter* p = request->getParam(i);
    message += "<div class='alert alert-primary'>" + String(p->name()) + ": " + String(p->value()) + "</div>";
  }
  message += "<br/>";
  message += "Current state: " + String(currentState);
  message += "<br/>";
  message += "Brightness: " + String(currentBrightness);
  message += "</div></body></html>";

  request->send(404, "text/html", message);
}

void handleRoot(AsyncWebServerRequest *request) {
  if (request->hasArg("state")) {
    int val = request->arg("state").toInt();
    setState(val);
  }
  State &state = getNState();

  if (request->hasArg("brightness")) {
    int val = request->arg("brightness").toInt();
    setBrightness(val);
  }

  if (request->hasArg("setType")) {
    int val = request->arg("setType").toInt();
    state.setType(val);
    setState(currentState);
  }
  if (request->hasArg("ts")) {
    state.setGammaCorrect(request->hasArg("gamma"));
    if (request->arg("ts") == "sc") {
      int r = request->arg("0R").toInt();
      int g = request->arg("0G").toInt();
      int b = request->arg("0B").toInt();
      int w = request->arg("0W").toInt();
      state.setColor(0, RgbwColor(g, r, b, w));

    } else if (request->arg("ts").equals("dct")) {
      int dayTemp = request->arg("dt").toInt();
      int nightTemp = request->arg("nt").toInt();
      String det = request->arg("det");
      int dayTime = 60 * (det.toInt()) + det.substring(3).toInt();
      String nit = request->arg("nit");
      int nightTime = 60 * (nit.toInt()) + nit.substring(3).toInt();
      int transTime = request->arg("tti").toInt();
      state.setDynTemp(dayTemp, nightTemp, dayTime, nightTime, transTime);

    } else if (request->arg("ts").equals("sp")) {
      for (int i = 0; i < state.colorCount; i++) {
        int r = request->arg(String(i) + "R").toInt();
        int g = request->arg(String(i) + "G").toInt();
        int b = request->arg(String(i) + "B").toInt();
        int w = request->arg(String(i) + "W").toInt();
        state.setColor(i, RgbwColor(g, r, b, w));
      }
    } else if (request->arg("ts").equals("rf")) {
      state.setAnimationDelay(request->arg("rft").toInt());
    }
    setState(currentState);
  }

  if (request->hasArg("save")) {
    saveStates();
  } else if (request->hasArg("load")) {
    loadStates();
  } else if (request->hasArg("clear")) {
    clearSavedStates();
  }

  setupTypes();

  String message =
    "<!DOCTYPE html>"
    "<head><title>Room Light Control</title><link rel='stylesheet' href='https://stackpath.bootstrapcdn.com/bootstrap/4.1.3/css/bootstrap.min.css'></head>"
    "<body><div class='container'>"
    "<h1 class='col'>Light control</h1>"
    "<div class='col'>"
    "<h2>Set state</h2>"
    "<a href='?state=0' class='btn btn-dark btn-lg btn-block'>Off</a>";
  for (int i = 1; i < stateCount; i++) {
    message += "<a href='?state=" + String(i) + "' class='btn btn-primary btn-lg btn-block ";
    if (i == targetState) {
      message += "active";
    }
    message += "'>State " + String(i) + "</a>";
  }
  message +=
    "</div>"
    "<div class='col'><h2>Brightness</h2><form method='post'><div class='row'>"
    "<div class='col-auto'><input type='range' name='brightness' min='0' max='255' value='" + String(getBrightness()) + "'/></div>"
    "<div class='col'><button type='submit' class='btn btn-primary'>Set</button></div>"
    "</div></form></div>"
    "<div class='col'><h2>Customize State " + String(targetState) + "</h2><div class='row'>"
    "<div class='col-md-4'>"
    "<h3>Type</h3>";
  for (int i = 0; i < typeCount; i++) {
    message += "<a href='?setType=" + String(i) + "' class='btn btn-primary btn-block ";
    if (i == state.getType()) {
      message += "active";
    }
    message += "'>" + types[i].getDesc() + "</a>";
  }
  message +=
    "</div><div class='col-md-8'><h3>Set properties</h3><form method='post'>";
  message += types[state.getType()].getSettingsHTML();
  message += "<label>Gamma correction: </label><input type='checkbox' name='gamma' ";
  if (state.getGammaCorrect()) {
    message += "checked";
  }
  message += "/><br/><button type='submit' class='btn btn-primary'>Submit</button>"
             "</form>"
             "</div></div></div>";
  message +=
    "<div class='col'><h2>Static memory</h2>"
    "<a href='?save' class='btn btn-primary btn-lg' >Save to</a> "
    "<a href='?load' class='btn btn-primary btn-lg' >Load from</a> "
    "<a href='?clear' class='btn btn-primary btn-lg' >Clear</a>"
    "</div><div class='col'><h2>Info</h2>";
  struct tm timeinfo;
  getLocalTime(&timeinfo);
  message += "Time: " + String(timeinfo.tm_hour) + ":" + String(timeinfo.tm_min) + ":" + String(timeinfo.tm_sec) + "<br/>";
  message += "Local ip: " + WiFi.localIP().toString();
  message +=  "</div></div></body></html>";

  request->send(200, "text/html", message);
}

void handleExternal(AsyncWebServerRequest *request) {
  if (request->hasArg("click")) {
    int clickVal = request->arg("click").toInt();
    if (currentState == clickVal) {
      setState(0);
    } else {
      setState( clickVal);
    }
  }

  if (request->hasArg("increment")) {
    int brightness = getBrightness() + request->arg("increment").toInt();
    if (brightness > 255) {
      brightness = 255;
    }
    setBrightness(brightness);
  }

  if (request->hasArg("decrement")) {
    int brightness = getBrightness() - request->arg("decrement").toInt();
    if (brightness < 0) {
      brightness = 0;
    }
    setBrightness(brightness);
  }
  request->send(200);
}

//State handling
void setState(int state) {
  if (targetState != state) {
    targetState = state;
    outsetBrightness = currentBrightness;
    animations.StartAnimation(1, 250, TransitionAnimUpdate);
  } else {
    animations.StopAnimation(0);
    int typeNumber = getState().getType();
    animations.StartAnimation(0, getState().getAnimationDelay(), types[typeNumber].getFunction());
  }
}

State& getState() {
  return states[currentState];
}

State& getNState() {
  return states[targetState];
}

//Brightness handling
void setBrightness(int target) {
  targetBrightness = float(target);
  outsetBrightness = currentBrightness;
  animations.StartAnimation(1, 250, BrightnessAnimUpdate);
}

int getBrightness() {
  return int(targetBrightness);
}

//Static storage tools
void setupStates() {
  preferences.begin("rlc", true);
  bool isFresh = preferences.getBool("fresh", true);
  preferences.end();
  if (isFresh) {
    generateStateDefaults();
    saveStates();
    preferences.begin("rlc", false);
    preferences.putBool("fresh", false);
    preferences.end();
  } else {
    loadStates();
  }
}

void saveStates() {
  for (int i = 0; i < stateCount; i++) {
    char iStr[3];
    sprintf(iStr, "%d", i);
    char ns[5] = "s";
    strcat(ns, iStr);
    preferences.begin(ns, false);
    State state = states[i];

    preferences.putInt("type", state.getType());

    for (int j = 0; j < state.colorCount; j++) {
      char jStr[3];
      sprintf(jStr, "%d", j);
      char vn[5];

      strcpy(vn, "R");
      strcat(vn, jStr);
      preferences.putInt(vn, state.getColor(j).G);
      strcpy(vn, "G");
      strcat(vn, jStr);
      preferences.putInt(vn, state.getColor(j).R);
      strcpy(vn, "B");
      strcat(vn, jStr);
      preferences.putInt(vn, state.getColor(j).B);
      strcpy(vn, "W");
      strcat(vn, jStr);
      preferences.putInt(vn, state.getColor(j).W);
    }

    preferences.putUInt("dayTemp", state.getDayTemp());
    preferences.putUInt("nightTemp", state.getNightTemp());
    preferences.putUInt("dayTime", state.getDayTime());
    preferences.putUInt("nightTime", state.getNightTime());
    preferences.putUInt("transTime", state.getTransTime());

    preferences.putUInt("animDelay", state.getAnimationDelay());

    preferences.putBool("gamma", state.getGammaCorrect());

    preferences.end();
  }
}

void loadStates() {
  for (int i = 0; i < stateCount; i++) {
    char iStr[3];
    sprintf(iStr, "%d", i);
    char ns[5] = "s";
    strcat(ns, iStr);
    preferences.begin(ns, true);
    State state = states[i];

    states[i].setType(preferences.getInt("type", state.getType()));

    for (int j = 0; j < state.colorCount; j++) {
      char jStr[3];
      sprintf(jStr, "%d", j);
      char vn[5];

      strcpy(vn, "R");
      strcat(vn, jStr);
      int g = preferences.getInt(vn, state.getColor(j).G);
      strcpy(vn, "G");
      strcat(vn, jStr);
      int r = preferences.getInt(vn, state.getColor(j).R);
      strcpy(vn, "B");
      strcat(vn, jStr);
      int b = preferences.getInt(vn, state.getColor(j).B);
      strcpy(vn, "W");
      strcat(vn, jStr);
      int w = preferences.getInt(vn, state.getColor(j).W);
      states[i].setColor(j, RgbwColor(r, g, b, w));
    }

    unsigned int dayTemp = preferences.getUInt("dayTemp", state.getDayTemp());
    unsigned int nightTemp = preferences.getUInt("nightTemp", state.getNightTemp());
    unsigned int dayTime = preferences.getUInt("dayTime", state.getDayTime());
    unsigned int nightTime = preferences.getUInt("nightTime", state.getNightTime());
    unsigned int transTime = preferences.getUInt("transTime", state.getTransTime());
    states[i].setDynTemp(dayTemp, nightTemp, dayTime, nightTime, transTime);

    states[i].setAnimationDelay(preferences.getUInt("animDelay", state.getAnimationDelay()));

    states[i].setGammaCorrect(preferences.getBool("gamma", state.getGammaCorrect()));

    preferences.end();
  }
}

void clearSavedStates() {
  preferences.begin("rlc", false);
  preferences.clear();
  preferences.end();

  for (int i = 0; i < stateCount; i++) {
    char iStr[3];
    sprintf(iStr, "%d", i);
    char ns[5] = "s";
    strcat(ns, iStr);
    preferences.begin(ns, false);
    preferences.clear();
    preferences.end();
  }
}

void generateStateDefaults() {
  states[0].setType(0);
  states[0].setColor(0, RgbwColor(0, 0, 0, 0));

  states[1].setType(1);
  states[1].setDynTemp(650, 0, 8 * 60, 22 * 60, 60);

  states[2].setType(0);
  states[2].setColor(0, RgbwColor(0, 255, 0, 0));

  states[2].setType(2);
  states[2].setColor(0, RgbwColor(0, 255, 0, 0));
  states[2].setColor(1, RgbwColor(0, 255, 0, 0));
  states[2].setColor(2, RgbwColor(0, 255, 0, 0));
  states[2].setColor(3, RgbwColor(0, 255, 0, 0));
  states[2].setColor(4, RgbwColor(0, 255, 255, 0));
  states[2].setColor(5, RgbwColor(0, 255, 255, 0));
  states[2].setColor(6, RgbwColor(0, 255, 255, 0));
  states[2].setColor(7, RgbwColor(0, 255, 255, 0));
  states[2].setColor(8, RgbwColor(0, 0, 255, 0));
  states[2].setColor(9, RgbwColor(0, 0, 255, 0));
  states[2].setColor(10, RgbwColor(0, 0, 255, 0));
  states[2].setColor(11, RgbwColor(0, 0, 255, 0));

  states[4].setType(3);
  states[4].setGammaCorrect(true);
}

//Animations
void BrightnessAnimUpdate(const AnimationParam& param) {
  currentBrightness = (targetBrightness - outsetBrightness) * param.progress + outsetBrightness;
}

void TransitionAnimUpdate(const AnimationParam& param) {
  currentBrightness =  outsetBrightness - (outsetBrightness * param.progress);

  if (param.state == AnimationState_Completed)
  {
    currentState = targetState;
    animations.StopAnimation(0);

    int typeNumber = getState().getType();
    animations.StartAnimation(0, getState().getAnimationDelay(), types[typeNumber].getFunction());
    setBrightness(targetBrightness);
  }
}

void StaticColorAnimUpdate(const AnimationParam& param) {
  RgbwColor color = RgbwColor::LinearBlend(RgbwColor(0, 0, 0, 0), getState().getColor(0), currentBrightness / 255.0);
  if (getState().getGammaCorrect()) {
    color = colorGamma.Correct(color);
  }
  strip.ClearTo(color);

  if (param.state == AnimationState_Completed)
  {
    animations.RestartAnimation(param.index);
  }
}

void ColorTemperatureAnimUpdate(const AnimationParam& param) {
  int dayTemp = getState().getDayTemp();
  int nightTemp = getState().getNightTemp();

  int dayTime = getState().getDayTime();
  int nightTime = getState().getNightTime();
  int transTime = getState().getTransTime();

  int cTime = getMins();

  int currentTemp;
  if (cTime <= (dayTime - (transTime / 2)) || cTime >= (nightTime + (transTime / 2))) {
    currentTemp = nightTemp;
  } else if (cTime >= (dayTime + (transTime / 2)) && cTime <= (nightTime - (transTime / 2))) {
    currentTemp = dayTemp;
  } else if (cTime < (dayTime + (transTime / 2))) {
    currentTemp = map(cTime, dayTime - (transTime / 2), dayTime + (transTime / 2), nightTemp, dayTemp);
  } else {
    currentTemp = map(cTime, nightTime - (transTime / 2), nightTime + (transTime / 2), dayTemp, nightTemp);
  }

  int r, g, b, w;
  if (currentTemp < 255) {
    r = 255;
    b = 0;
    w = currentTemp;
  } else if (currentTemp < 255 * 2) {
    r = (255 * 2) - currentTemp;
    b = 0;
    w = 255;
  } else {
    r = 0;
    b = currentTemp - 255 * 2;
    w = 255;
  }
  g = map(gwl, rwl, bwl, r, b);

  RgbwColor color = RgbwColor::LinearBlend(RgbwColor(0, 0, 0, 0), RgbwColor(g, r, b, w), currentBrightness / 255.0);

  if (getState().getGammaCorrect()) {
    color = colorGamma.Correct(color);
  }
  strip.ClearTo(color);

  if (param.state == AnimationState_Completed)
  {
    animations.RestartAnimation(param.index);
  }
}

void StaticPatternAnimUpdate(const AnimationParam& param) {
  int cc = getState().colorCount;
  int segLen = PixelCount / cc;
  for (int i = 0; i < cc; i++) {
    RgbwColor color = RgbwColor::LinearBlend(RgbwColor(0, 0, 0, 0), getState().getColor(i), currentBrightness / 255.0);
    if (getState().getGammaCorrect()) {
      color = colorGamma.Correct(color);
    }
    strip.ClearTo(color, i * segLen, (i + 1)*segLen);
  }

  if (param.state == AnimationState_Completed)
  {
    animations.RestartAnimation(param.index);
  }
}

void RainbowAnimUpdate(const AnimationParam& param){
  for (int i = 0; i < PixelCount; i++) {
    float progress = (param.progress + (float(i) / float(PixelCount)));
    while (progress > 1.0f) {
      progress -= 1.0f;
    }
    RgbwColor color = RgbwColor(HslColor(progress, 1.0f, currentBrightness / 255.0));
    if (getState().getGammaCorrect()) {
      color = colorGamma.Correct(color);
    }
    strip.SetPixelColor(i, color);
  }
  if (param.state == AnimationState_Completed)
  {
    animations.RestartAnimation(param.index);
  }
}

//Tools
int getMins() {
  struct tm timeinfo;
  getLocalTime(&timeinfo);
  return timeinfo.tm_hour * 60 + timeinfo.tm_min;
}

String getTimeString(unsigned int mins) {
  String hours = String((mins - (mins % 60)) / 60);
  if (hours.length() < 2) {
    hours = "0" + hours;
  }
  String minutes = String(mins % 60);
  if (minutes.length() < 2 ) {
    minutes = "0" + minutes;
  }
  return hours + ":" + minutes;
}
