/*
 *  HomeLight ESP8266 - RGBW Variant - Firmware for Desk
 *  Created on: 2021-07-28
 *  Author: Ben Gross
 */

#include                              <Arduino.h>
#include                              <ESP8266WiFi.h>
#include                              <ESP8266mDNS.h>
#include                              <WebSocketsServer.h>
#include                              <SoftPin.h>
#include                              <SoftLight.h>
#include                              "WifiCredentials.h"
#include                              <FastLED.h>
#include                              "FastLED_RGBW.h"


#define DATA_PIN D4
#define NUM_LEDS 143



WebSocketsServer webSocket(81);    // create a websocket server on port 81
CRGBW leds[NUM_LEDS];
CRGB *ledsRGB = (CRGB *) &leds[0];

const char* mdnsName = "esp8266"; // Domain name for the mDNS responder
const int deviceType = 1;

unsigned long lastReconnectMillis = 0;
unsigned long lastFrameMillis = 0;

int maxFrames = 1500;
const int setSize = 8;
const int numberOfSets = NUM_LEDS / setSize;

char currentMode = '#';

CHSV currentColorHSV = CHSV(0,0,0);
CRGBW tempRGBW = CRGBW(0,0,0,0);
CRGB tempRGB = CRGB(0,0,0);
int currentWhite = 0;


void setup() {
  Serial.begin(115200);

   //---- pixel config and initializing to black:
  FastLED.addLeds<WS2812B, DATA_PIN, RGB>(ledsRGB, getRGBWsize(NUM_LEDS));
  for(int i = 0; i < NUM_LEDS; i++){
    leds[i] = CRGB::Black;
  }
  FastLED.setBrightness(255);
  FastLED.show();

  // WIFI Setup:
  WiFi.mode(WIFI_STA);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.begin(ssid, password);
  Serial.println("WiFi connecting...");
  while (!WiFi.isConnected()) {
    delay(100);
    Serial.print(".");
  }
  Serial.print("\n");
  Serial.printf("WiFi connected, IP: %s\n", WiFi.localIP().toString().c_str());
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);

  MDNS.begin(mdnsName);                        // start the multicast domain name server
  webSocket.begin();                          // start the websocket server
  webSocket.onEvent(webSocketEvent);          // if there's an incomming websocket message, go to function 'webSocketEvent' 
  lastReconnectMillis = millis();
}



void loop() {
  if(WiFi.status() != WL_CONNECTED && (millis() - lastReconnectMillis) > 5000)
  {
    lastReconnectMillis = millis(); 
    WiFi.reconnect();
  }
  webSocket.loop();

  
  switch(currentMode){
    case '#':
      tempRGB = CRGB(0,0,0);
      hsv2rgb_rainbow(currentColorHSV, tempRGB);
      tempRGBW = CRGBW(tempRGB.r, tempRGB.g, tempRGB.b, currentWhite);
      for(int i = 0; i < NUM_LEDS; i++){
        leds[i] = tempRGBW;
      }
      break;

    case 'A':
      break;

    case 'B':
      int frame = millis() % maxFrames;
      float offset = (float)frame / (float)maxFrames;
      //direction modifier (-1): offset * 1 OR offset * -1
      renderWave(offset * -1);
      break;
  }
  

  if(millis() - lastFrameMillis > 50){
    lastFrameMillis = millis();
    FastLED.show();
  }
}



void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t messageLength) { // When a WebSocket message is received
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED: {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
      }
      break;
    case WStype_TEXT:
      if(messageLength < 1){
        Serial.println("Incomplete Message:");
        Serial.printf("[%u] get Text: %s\n", num, payload);
        return;
      }
      Serial.printf("[%u] get Text: %s\n", num, payload);
      
      if (payload[0] == '#') {
        uint32_t hsv = (uint32_t) strtoul((const char *) &payload[1], NULL, 16);   // decode rgb data
        int h = ((hsv >> 24) & 0xFF);
        int s = ((hsv >> 16) & 0xFF);
        int v = ((hsv >>  8) & 0xFF);
        int w =          hsv & 0xFF;
        Serial.print("White: ");
        Serial.print(w);
        Serial.println("");
        Serial.print("Hue: ");
        Serial.print(h);
        Serial.println("");
        Serial.print("Sat: ");
        Serial.print(s);
        Serial.println("");
        Serial.print("Val: ");
        Serial.print(v);
        Serial.println("");
        currentColorHSV = CHSV(h, s, v);
        currentWhite = w;
      } 
      
      else if (payload[0] == '$') {
        String response = "R";
        response += deviceType;
        response += String(currentMode);
        if(currentColorHSV.hue < 0x0F){
          response += "0";
        }
        response += String(currentColorHSV.hue, HEX);
        if(currentColorHSV.sat < 0x0F){
          response += "0";
        }
        response += String(currentColorHSV.sat, HEX);
        if(currentColorHSV.val < 0x0F){
          response += "0";
        }
        response += String(currentColorHSV.val, HEX);
        if(currentWhite < 0x0F){
          response += "0";
        }
        response += String(currentWhite, HEX);
        Serial.println(response);
        webSocket.sendTXT(num, response);
      } 
      
      else if (payload[0] == 'A') {

      }

      else if (payload[0] == 'B') {
        uint32_t hsv = (uint32_t) strtol((const char *) &payload[1], NULL, 16);   // decode rgb data
        int h = ((hsv >> 16) & 0xFF);
        int s = ((hsv >>  8) & 0xFF);
        int v =          hsv & 0xFF;
        currentColorHSV = CHSV(h,s,v);
        currentMode = 'B';
      }
      
      
      break;
  }
}




/**
 * renders Wave LED strip. Iterates every set of LEDs, and LED per set.
 * float offset: 0-0.999... offset of period
 */
void renderWave(float offset){
  for(int i = 0; i <= numberOfSets; i++){
    for (int ii = 1; ii <= setSize; ii++){
      if(i * setSize + ii <= NUM_LEDS){
        float ledValueMod = ii + (setSize * offset);
        uint8_t pos = ledValueMod * (256 / setSize);
        leds[i * setSize + ii - 1] = CHSV(currentColorHSV.hue, currentColorHSV.saturation, levelCalculationWave(pos));
      }
    }
  }  
}


/**
 * Calculates actual value for LED from a defined function.
 * pos: 0-255 sweep position for LED
 */
int levelCalculationWave(uint8_t pos){
  return (-0.6 * sin(6.28 * pos / 256) * (pos-256)) + 128;
}
