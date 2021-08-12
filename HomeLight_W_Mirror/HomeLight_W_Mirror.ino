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
#include                              "WifiCredentials.h"


#define DATA_PIN D6



WebSocketsServer webSocket(81);    // create a websocket server on port 81

const char* mdnsName = "mirror"; // Domain name for the mDNS responder
const int deviceType = 2;

SoftPin w_pin(DATA_PIN, 500, true);

unsigned long lastReconnectMillis = 0;
unsigned long lastFrameMillis = 0;

char currentMode = '#';

int h = 0;
int s = 0;
int v = 0;
int w = 0;

void setup() {
  pinMode(D4, OUTPUT);
  digitalWrite(D4, HIGH);
  Serial.begin(115200);

  pinMode(DATA_PIN, OUTPUT);
  digitalWrite(DATA_PIN, LOW);
  analogWriteRange(1023);

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
  w_pin.loop();
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
        v = ((hsv >>  8) & 0xFF);
        Serial.println(v);
        w_pin.set255(v);
      } 
      
      else if (payload[0] == '$') {
        String response = "R";
        response += deviceType;
        response += String(currentMode);
        if(h < 0x0F){
          response += "0";
        }
        response += String(h, HEX);
        if(s < 0x0F){
          response += "0";
        }
        response += String(s, HEX);
        if(v < 0x0F){
          response += "0";
        }
        response += String(v, HEX);
        if(w < 0x0F){
          response += "0";
        }
        response += String(w, HEX);
        Serial.println(response);
        webSocket.sendTXT(num, response);
      } 
      
      break;
  }
}
