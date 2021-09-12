/*
 *  HomeLight ESP8266 - RGBW Variant - Firmware for Desk
 *  Created on: 2021-07-28
 *  Author: Ben Gross
 */

#include                              <Arduino.h>
#include                              <ESP8266WiFi.h>
#include                              <ESP8266mDNS.h>
#include                              <WebSocketsServer.h>
#include                              "WifiCredentials.h"
#include                              <Adafruit_NeoPixel.h>
#include                              <SoftValue.h>



#define DATA_PIN D2
#define NUM_LEDS 143



WebSocketsServer webSocket(81);    // create a websocket server on port 81
Adafruit_NeoPixel strip(NUM_LEDS, DATA_PIN, NEO_GRBW + NEO_KHZ800);
SoftValue softH(500);
SoftValue softS(500);
SoftValue softV(500);
SoftValue softW(500);

uint32_t color = 0;

const char* mdnsName = "desk"; // Domain name for the mDNS responder
const int deviceType = 1;

unsigned long lastReconnectMillis = 0;unsigned long lastFrameMillis = 0;

int maxFrames = 1500;
const int setSize = 8;
const int numberOfSets = NUM_LEDS / setSize;

char currentMode = '#';


void setup() {
  pinMode(D4, OUTPUT);
  digitalWrite(D4, HIGH);
  Serial.begin(115200);

  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  
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
  MDNS.update();

  softH.loop();
  softS.loop();
  softV.loop();
  softW.loop();

  if(currentMode == '#'){
      color = strip.ColorHSV(softH.getCurrentValue() * 255, softS.getCurrentValue(), softV.getCurrentValue());
      color = (softW.getCurrentValue() << 24) | color;

      
      strip.fill(color);
    }
    else if(currentMode == 'A'){
    int frame = millis() % maxFrames;
        float offset = (float)frame / (float)maxFrames;
        //direction modifier (-1): offset * 1 OR offset * -1
        renderWaveFlag(offset * -1);
    }
    else if(currentMode == 'B'){
      int frame = millis() % maxFrames;
        float offset = (float)frame / (float)maxFrames;
        //direction modifier (-1): offset * 1 OR offset * -1
        renderWave(offset * -1);
    }

  if(millis() - lastFrameMillis > 30){
    lastFrameMillis = millis();
    strip.show();
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
        currentMode = '#';
        uint32_t hsv = (uint32_t) strtoul((const char *) &payload[1], NULL, 16);   // decode rgb data
        softH.setValue(((hsv >> 24) & 0xFF));
        softS.setValue(((hsv >> 16) & 0xFF));
        softV.setValue(((hsv >>  8) & 0xFF));
        softW.setValue(         hsv & 0xFF);
      } 
      
      else if (payload[0] == '$') {
        String response = "R";
        response += deviceType;
        response += String(currentMode);
        if(softH.getTargetValue() < 0x0F){
          response += "0";
        }
        response += String(softH.getTargetValue(), HEX);
        if(softS.getTargetValue() < 0x0F){
          response += "0";
        }
        response += String(softH.getTargetValue(), HEX);
        if(softV.getTargetValue() < 0x0F){
          response += "0";
        }
        response += String(softV.getTargetValue(), HEX);
        if(softW.getTargetValue() < 0x0F){
          response += "0";
        }
        response += String(softW.getTargetValue(), HEX);
        Serial.println(response);
        webSocket.sendTXT(num, response);
      } 
      
      else if (payload[0] == 'A') {
        currentMode = 'A';
        uint32_t hsv = (uint32_t) strtoul((const char *) &payload[1], NULL, 16);   // decode rgb data
        softH.setValue(((hsv >> 24) & 0xFF));
        softS.setValue(((hsv >> 16) & 0xFF));
        softV.setValue(((hsv >>  8) & 0xFF));
        softW.setValue(         hsv & 0xFF);
      }

      else if (payload[0] == 'B') {
        uint32_t hsv = (uint32_t) strtol((const char *) &payload[1], NULL, 16);   // decode rgb data
        softH.setValue(((hsv >> 24) & 0xFF));
        softS.setValue(((hsv >> 16) & 0xFF));
        softV.setValue(((hsv >>  8) & 0xFF));
        softW.setValue(         hsv & 0xFF);
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
        int ledPos = i * setSize + ii - 1;
        color = strip.ColorHSV((uint16_t)softH.getCurrentValue() * 255, softS.getCurrentValue(), levelCalculationWave(pos));
        strip.setPixelColor(ledPos, color);
      }
    }
  }
}


/**
 * renders Wave LED strip. Iterates every set of LEDs, and LED per set.
 * float offset: 0-0.999... offset of period
 */
void renderWaveFlag(float offset){
  for(int i = 0; i <= numberOfSets; i++){
    for (int ii = 1; ii <= setSize; ii++){
      if(i * setSize + ii <= NUM_LEDS){
        float ledValueMod = ii + (setSize * offset);
        uint8_t pos = ledValueMod * (256 / setSize);
        int ledPos = i * setSize + ii - 1;
        
        if(ledPos < 35){
          color = strip.ColorHSV(40800, 255, levelCalculationWave(pos));
          strip.setPixelColor(ledPos, color);
        }
        else if(ledPos >= 35 && ledPos < 89){
          color = strip.ColorHSV(0, 255, levelCalculationWave(pos));
          strip.setPixelColor(ledPos, color);
        }
        else if(ledPos >= 89 ){
          color = strip.ColorHSV(0, 0, levelCalculationWave(pos));
          strip.setPixelColor(ledPos, color);
        }
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
