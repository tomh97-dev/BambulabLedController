#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <cstring>
#include <PubSubClient.h>
#include <ArduinoJson.h> 
#include <EEPROM.h>
#include "eeprom_utils.h"
#include "led_utils.h"
// #include "ws2812_utils.h"
#include "variables.h"
#include "html.h"
#include <FastLED.h>

#define MAX_LEDS 300
#define DATA_PIN D4
CRGB leds[MAX_LEDS];

const char* wifiname = "Bambulab Led controller";
const char* setuppage = html_setuppage;
const char* finishedpage = html_finishpage;

char Printerip[Max_ipLength+1] = "";
char Printercode[Max_accessCode+1] = ""; 
char PrinterID[Max_DeviceId+1] = "";
char EspPassword[Max_EspPassword+1] = "";
int LedType = 1;
int LedCount = 16;
char DeviceName[20];

LedState currentLedState = IDLE;
int CurrentStage = -1;
bool isPrinting = false;
bool isHeating = false;
float bedTargetTemp = 0;
float nozzleTargetTemp = 0;
float lastBedTemp = 0.0;
float lastNozzleTemp = 0.0;
bool hasHMSerror = false;
bool ledstate = false;
unsigned long finishstartms;
unsigned long lastmqttconnectionattempt;

ESP8266WebServer server(80);
IPAddress apIP(192, 168, 1, 1);

WiFiClientSecure WiFiClient;
WiFiManager wifiManager;
PubSubClient mqttClient(WiFiClient);

char* generateRandomString(int length) {
  static const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
  int charsetLength = strlen(charset);

  char* randomString = new char[length + 1];
  for (int i = 0; i < length; i++) {
    int randomIndex = random(0, charsetLength);
    randomString[i] = charset[randomIndex];
  }
  randomString[length] = '\0';

  return randomString;
}

void updateFastLED() {
  static unsigned long lastUpdate = 0;
  const long interval = 1000; // Example interval

  if (millis() - lastUpdate >= interval) {
    lastUpdate = millis();
    if (ledstate == 1 && currentLedState != ERROR) { // Always illuminate for errors?
      switch (currentLedState) {
        case IDLE:
          fill_solid(leds, LedCount, CRGB(255, 255, 255));
          FastLED.show();
          break;
        case PREHEATING:
          fadeAnimation(255,113,0);
          break;
        case PRINTING:
          fill_solid(leds, LedCount, CRGB(255, 255, 255));
          FastLED.show();
          break;
        case ERROR:
          fadeAnimation(255,0,0);
          break;
        case ERROR_RESOLVED:
          fadeAnimation(255,0,0);
          fadeAnimation(0,255,0);
          break;
        case PRINT_COMPLETE:
          for (int i=0; i > 10; i++) fadeAnimation(0,255,0);
          break;
        case OFF:
          fill_solid(leds, LedCount, CRGB::Black);
          FastLED.show();
          break;
        default:
          fill_solid(leds, LedCount, CRGB::White);
          FastLED.show();
      }
    }
    else
    {
      fill_solid(leds, LedCount, CRGB::Black);
      FastLED.show();
    }
  }
}

void updateRGBWLed(){ //Function to handle ledstatus eg if the X1C has an error then make the ledstrip red, or when its scanning turn off the light until its starts printing
static unsigned long lastUpdate = 0;
  const long interval = 500; // Example interval

  if (millis() - lastUpdate >= interval) {
    lastUpdate = millis();
    if (ledstate == 1 && currentLedState != ERROR) { // Always illuminate for errors?
        switch (currentLedState) {
          case IDLE:
            setLedColor(0,0,0,255,255);
            break;
          case PREHEATING:
            setLedColor(255,113,0,0,0);
            break;
          case PRINTING:
            setLedColor(0,0,0,255,255);
            break;
          case ERROR:
            setLedColor(255,0,0,0,0);
            break;
          // case ERROR_RESOLVED:
          //   setLedColor(0,0,0,255,255);
          //   break;
          case PRINT_COMPLETE:
            setLedColor(0,255,0,0,0);
            break;
          case OFF:
            setLedColor(0,0,0,0,0);
            break;
          default:
            fill_solid(leds, LedCount, CRGB::White);
            FastLED.show();
        }
      }
      else setLedColor(0,0,0,0,0);
  }
}

void parseStages() {
  if (CurrentStage == 6 || CurrentStage == 17 || CurrentStage == 20 || CurrentStage == 21 || hasHMSerror){
    currentLedState = ERROR;
    return;
  };
  if (isPrinting)
  {
    currentLedState = PRINTING;
    return;
  }
  if (isHeating)
  {
    currentLedState = PREHEATING;
    return;
  }
  if (CurrentStage == 255)
  {
    currentLedState = PRINT_COMPLETE;
    return;
  }
  if (finishstartms > 0 && millis() - finishstartms <= 300000){
    currentLedState = PRINT_COMPLETE;
    return;
  }else if(millis() - finishstartms > 300000){
    finishstartms;
  }
  if (CurrentStage == 0 || CurrentStage == -1 || CurrentStage == 2){
    currentLedState = IDLE;
    return;
  };
  if (CurrentStage == 14 || CurrentStage == 9){
    currentLedState = OFF;
    return;
  };
  Serial.println(currentLedState);
}

void fadeAnimation(int red, int green, int blue){
  float r, g, b;
  // FADE IN
  for(int i = 0; i <= 255; i++) {
    r = (i/256.0)*red;
    g = (i/256.0)*green;
    b = (i/256.0)*blue;
    fill_solid(leds, LedCount, CRGB(r, g, b));
    FastLED.show();
    delay(2);
  }

  // FADE OUT
  for(int i = 255; i >= 0; i--) {
    r = (i/256.0)*red;
    g = (i/256.0)*green;
    b = (i/256.0)*blue;
    fill_solid(leds, LedCount, CRGB(r, g, b));
    FastLED.show();
    delay(2);
  }
}

void replaceSubstring(char* string, const char* substring, const char* newSubstring) {
    char* substringStart = strstr(string, substring);
    if (substringStart) {
        char* substringEnd = substringStart + strlen(substring);
        memmove(substringStart + strlen(newSubstring), substringEnd, strlen(substringEnd) + 1);
        memcpy(substringStart, newSubstring, strlen(newSubstring));
    }
}

void handleSetTemperature() {
  if (!server.hasArg("api_key")) {
    return server.send(400, "text/plain", "Missing API key parameter.");
  };
  char shortened_key[7];
  strncpy(shortened_key, EspPassword, 7);
  shortened_key[7] = '\0'; 
  char received_api_key[8];

  server.arg("api_key").toCharArray(received_api_key, 8);
  if (!strcmp(received_api_key, shortened_key) == 0) {
    return server.send(401, "text/plain", "Unauthorized access.");
  }
  char mqttTopic[50];
  strcpy(mqttTopic, "device/");
  strcat(mqttTopic, PrinterID);
  strcat(mqttTopic, "/request");
  if (server.hasArg("bedtemp")) {
    float bedtemp = server.arg("bedtemp").toFloat();
    String message = "{\"print\":{\"sequence_id\":\"2026\",\"command\":\"gcode_line\",\"param\":\"M140 S" + String(bedtemp) + "\\n\"}}";
    mqttClient.publish(mqttTopic, message.c_str());
  }
  if (server.hasArg("nozzletemp")) {
    float bedtemp = server.arg("nozzletemp").toFloat();
    String message = "{\"print\":{\"sequence_id\":\"2026\",\"command\":\"gcode_line\",\"param\":\"M104 S" + String(bedtemp) + "\\n\"}}";
    mqttClient.publish(mqttTopic, message.c_str());
  }
  return server.send(200, "text/plain", "Ok");
}

void handleSetupRoot() { //Function to handle the setuppage
  if (!server.authenticate("BLLC", EspPassword)) {
    return server.requestAuthentication();
  }
  replaceSubstring((char*)setuppage, "ipinputvalue", Printerip);
  replaceSubstring((char*)setuppage, "idinputvalue", PrinterID);
  replaceSubstring((char*)setuppage, "codeinputvalue", Printercode);
  if (LedType == 1) {
      replaceSubstring((char*)setuppage, "%%RGBW_SELECTED%%", "selected");
      replaceSubstring((char*)setuppage, "%%WS2812_SELECTED%%", "");
  }else if (LedType == 2) {
      replaceSubstring((char*)setuppage, "%%RGBW_SELECTED%%", "");
      replaceSubstring((char*)setuppage, "%%WS2812_SELECTED%%", "selected");
  }
  // Buffer to hold the string representation of LedCount
  char ledCountStr[10]; // Make sure this buffer is large enough to hold the number

  // Convert LedCount to a string
  sprintf(ledCountStr, "%d", LedCount);
  replaceSubstring((char*)setuppage, "ledcountvalue", ledCountStr);
  server.send(200, "text/html", setuppage);
}

void SetupWebpage(){ //Function to start webpage system
  Serial.println(F("Starting Web server"));
  server.on("/", handleSetupRoot);
  server.on("/setupmqtt", savemqttdata);
 // server.on("/settemp", handleSetTemperature);
  server.begin();
  Serial.println(F("Web server started"));
}

void savemqttdata() {
  char iparg[Max_ipLength + 1];
  char codearg[Max_accessCode + 1];
  char idarg[Max_DeviceId + 1];

  // Copy the arguments from server to char arrays
  server.arg("ip").toCharArray(iparg, Max_ipLength + 1);
  server.arg("code").toCharArray(codearg, Max_accessCode + 1);
  server.arg("id").toCharArray(idarg, Max_DeviceId + 1);
  int ledarg = server.arg("led").toInt();
  int ledcountarg = server.arg("ledcount").toInt();
  if (strlen(iparg) == 0 || strlen(codearg) == 0 || strlen(idarg) == 0) {
    return handleSetupRoot();
  }
  server.send(200, "text/html", finishedpage);

  Serial.println(F("Printer IP:"));
  Serial.println(iparg);
  Serial.println(F("Printer Code:"));
  Serial.println(codearg);
  Serial.println(F("Printer Id:"));
  Serial.println(idarg);
  Serial.println(F("LED Type:"));
  Serial.println(ledarg);
  Serial.println(F("LED Count:"));
  Serial.println(ledcountarg);

  writeToEEPROM(iparg, codearg, idarg, EspPassword, &ledarg, &ledcountarg);
  delay(1000); //wait for page to load
  ESP.restart();
}


void PrinterCallback(char* topic, byte* payload, unsigned int length){ //Function to handle the MQTT Data from the mqtt broker
  Serial.printf("Free heap: %u\n", ESP.getFreeHeap());
  Serial.print(F("Message arrived in topic: "));
  Serial.println(topic);
  Serial.print(F("Message Length: "));
  Serial.println(length);
  Serial.print(F("Message:"));

  StaticJsonDocument<10000> doc;
  DeserializationError error = deserializeJson(doc, payload, length);

  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }

   if (!doc.containsKey("print")) {
    return;
  }

  Serial.println(F("===== JSON Data ====="));
  serializeJsonPretty(doc, Serial);
  Serial.println(F("======================"));

  if (doc["print"].containsKey("stg_cur")){
    CurrentStage = doc["print"]["stg_cur"];
  }

  if (doc["print"].containsKey("bed_temper")){
    lastBedTemp = doc["print"]["bed_temper"];
  }
  if (doc["print"].containsKey("nozzle_temper")){
    lastNozzleTemp = doc["print"]["nozzle_temper"];
  }

  if (doc["print"].containsKey("bed_target_temper")){
    bedTargetTemp = doc["print"]["bed_target_temper"];
  }

  if (doc["print"].containsKey("nozzle_target_temper")){
    nozzleTargetTemp = doc["print"]["nozzle_target_temper"];
  }

  if (lastBedTemp != 0.0 && lastNozzleTemp != 0.0) isHeating = (bedTargetTemp > lastBedTemp + 1.0) || (nozzleTargetTemp > lastNozzleTemp + 1.0);

  if (doc["print"].containsKey("mc_print_line_number")){
    isPrinting = true;
  }
  else isPrinting = false;

  Serial.print(F("stg_cur: "));
  Serial.println(CurrentStage);

  if (doc["print"].containsKey("gcode_state")){
    if (doc["print"]["gcode_state"] == "FINISH" && finishstartms <= 0){
      finishstartms = millis();
    }else if (doc["print"]["gcode_state"] != "FINISH" && finishstartms > 0){
      finishstartms = 0;
    }
  }
  
  if (doc["print"].containsKey("hms")){
    hasHMSerror = false;
    for (const auto& hms : doc["print"]["hms"].as<JsonArray>()) {
        if (hms["code"] == 131073) {
          hasHMSerror = true;
        };
    }
  }

  Serial.print(F("HMS error: "));
  Serial.println(hasHMSerror);

  if (doc["print"].containsKey("lights_report")) {
    if (doc["print"]["lights_report"][0]["node"] == "chamber_light"){
      ledstate = doc["print"]["lights_report"][0]["mode"] == "on";
      Serial.print("Ledchanged: ");
      Serial.println(ledstate);
    }
  }

  Serial.print(F("cur_led: "));
  Serial.println(ledstate);

  Serial.print(F("is heating: "));
  Serial.println(isHeating);

  Serial.print(F("is printing: "));
  Serial.println(isPrinting);

  Serial.println(F(" - - - - - - - - - - - -"));
  parseStages();
}

void setup() { // Setup function
  Serial.begin(115200);
  EEPROM.begin(512);
  
  // clearEEPROM();

  WiFiClient.setInsecure();
  mqttClient.setBufferSize(10000);

  if (wifiManager.getWiFiIsSaved()) wifiManager.setEnableConfigPortal(false);
  wifiManager.autoConnect(wifiname);

  WiFi.hostname("bambuledcontroller");

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("Failed to connect to WiFi, creating access point..."));
    wifiManager.setAPCallback([](WiFiManager* mgr) {
      Serial.println(F("Access point created, connect to:"));
      Serial.print(mgr->getConfigPortalSSID());
    });
    wifiManager.setConfigPortalTimeout(300);
    wifiManager.startConfigPortal(wifiname);
  }

  while (WiFi.status() != WL_CONNECTED) {
    delay(10);
    Serial.println(F("Connecting to Wi-Fi..."));
  }

  readFromEEPROM(Printerip,Printercode,PrinterID,EspPassword,&LedType,&LedCount);

  if (strchr(EspPassword, '#') == NULL) { //Isue with eeprom giving �, so adding a # to check if the eeprom is empty or not
    Serial.println(F("No Password has been set, Resetting"));
    memset(EspPassword, 0, Max_EspPassword);
    memset(Printercode, '_', Max_accessCode);
    memset(PrinterID, '_', Max_DeviceId);
    memset(Printerip, '_', Max_ipLength);
    char* newEspPassword = generateRandomString(Max_EspPassword-1);
    strcat(newEspPassword, "#");
    strcat(EspPassword, newEspPassword);
    writeToEEPROM(Printerip, Printercode, PrinterID, EspPassword,&LedType,&LedCount);
    readFromEEPROM(Printerip,Printercode,PrinterID,EspPassword,&LedType,&LedCount); //This will auto clear the eeprom
  };

  Serial.print(F("Connected to WiFi, IP address: "));
  Serial.println(WiFi.localIP());
  Serial.println(F("-------------------------------------"));
  Serial.print(F("Head over to http://"));
  Serial.println(WiFi.localIP());
  Serial.print(F("Login Details User: BLLC, Password: "));
  Serial.println(String(EspPassword));
  Serial.println(F(" To configure the mqtt settings."));
  Serial.println(F("-------------------------------------"));

  SetupWebpage();

  strcpy(DeviceName, "ESP8266MQTT");
  char* randomString = generateRandomString(4);
  strcat(DeviceName, randomString);

  switch (LedType) {
    case 1:
        initialiseRGBW();
        break;
    case 2:
        initialiseFastLED();
        break;
    default:
        initialiseRGBW();
  } 

  mqttClient.setServer(Printerip, 8883);
  mqttClient.setCallback(PrinterCallback);

}

void initialiseFastLED() {
    Serial.println("Initialising WS2812 Driver...");
    if (LedCount > MAX_LEDS) {
        LedCount = MAX_LEDS;  // Cap the number of LEDs to the maximum
    }
    FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, LedCount);
    FastLED.setMaxPowerInVoltsAndMilliamps(5, 2000);
}

void loop() { //Loop function
  server.handleClient();
  if (WiFi.status() != WL_CONNECTED){
      Serial.println(F("Connection lost! Reconnecting..."));
      //wifiManager.autoConnect(wifiname);
      //Serial.println(F("Connected to WiFi!"));
      ESP.restart();
  }
  if (WiFi.status() == WL_CONNECTED && strlen(Printerip) > 0 && (lastmqttconnectionattempt <= 0 || millis() - lastmqttconnectionattempt >= 10000)){
    if (!mqttClient.connected()) {

      Serial.print(F("Connecting with device name:"));
      Serial.println(DeviceName);
      Serial.println(F("Connecting to mqtt"));
      
      if (mqttClient.connect(DeviceName, "bblp", Printercode)){
        Serial.println(F("Connected to MQTT"));
        if (LedType == 1) setLedColor(0,0,0,0,0); //Turn off led printer might be offline
        for(int i = 0; i < LedCount; i++) {leds[i] = CRGB::Black;}
        char mqttTopic[50];
        strcpy(mqttTopic, "device/");
        strcat(mqttTopic, PrinterID);
        strcat(mqttTopic, "/report");
        Serial.println("Topic: ");
        Serial.println(mqttTopic);
        mqttClient.subscribe(mqttTopic);
        lastmqttconnectionattempt;
      } else {
        if (LedType == 1) setPins(0,0,0,0,0);
        for(int i = 0; i < LedCount; i++) {leds[i] = CRGB::Black;}
        Serial.print("failed, rc=");
        Serial.print(mqttClient.state());
        Serial.println(" try again in 10 seconds");
        lastmqttconnectionattempt = millis();
      }
    }
  }
  updateFastLED();
  mqttClient.loop();
}