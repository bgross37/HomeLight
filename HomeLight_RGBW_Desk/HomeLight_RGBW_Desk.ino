/*
 *  HomeLight ESP8266 - RGBW Variant - Firmware for Desk
 *  Created on: 2021-07-28
 *  Author: Ben Gross
 */

#include                              <Arduino.h>
#include                              <ESP8266WiFi.h>
#include                              <SoftPin.h>
#include                              <SoftLight.h>
#include                              "WifiCredentials.h"
#include                              <FastLED.h>
#include                              "FastLED_RGBW.h"


#define DATA_PIN D4
#define NUM_LEDS 143

const uint8_t LUT[] = 
{64,66,67,69,70,72,73,75,
76,78,80,81,83,84,86,87,
88,90,91,93,94,96,97,98,
100,101,102,103,105,106,107,108,
109,110,111,112,113,114,115,116,
117,118,119,120,120,121,122,123,
123,124,124,125,125,126,126,126,
127,127,127,128,128,128,128,128,
128,128,128,128,128,128,127,127,
127,126,126,126,125,125,124,124,
123,123,122,121,120,120,119,118,
117,116,115,114,113,112,111,110,
109,108,107,106,105,103,102,101,
100,98,97,96,94,93,91,90,
88,87,86,84,83,81,80,78,
76,75,73,72,70,69,67,66,
64,62,61,59,58,56,55,53,
52,50,48,47,45,44,42,41,
40,38,37,35,34,32,31,30,
28,27,26,25,23,22,21,20,
19,18,17,16,15,14,13,12,
11,10,9,8,8,7,6,5,
5,4,4,3,3,2,2,2,
1,1,1,0,0,0,0,0,
0,0,0,0,0,0,1,1,
1,2,2,2,3,3,4,4,
5,5,6,7,8,8,9,10,
11,12,13,14,15,16,17,18,
19,20,21,22,23,25,26,27,
28,30,31,32,34,35,37,38,
40,41,42,44,45,47,48,50,
52,53,55,56,58,59,61,62};

SoftLight softlight(1);
//SoftAlarm softalarm(1023);
//SoftPin w_pin(LED_pin, 500);

CRGBW leds[NUM_LEDS];
CRGB *ledsRGB = (CRGB *) &leds[0];

unsigned long lastReconnectMillis = 0;


long currentFrame = millis();
int maxFrames = 1500;


const int setSize = 8;
const int numberOfSets = NUM_LEDS / setSize;

int hue = 0;
int sat = 200;


void setup() {
  Serial.begin(115200);

   //---- pixel config and initializing to black, sweeping and back to black:
  FastLED.addLeds<WS2812B, DATA_PIN, RGB>(ledsRGB, getRGBWsize(NUM_LEDS));
  for(int i = 0; i < NUM_LEDS; i++){
    leds[i] = CRGB::Black;
  }
  FastLED.setBrightness(255);
  FastLED.show();

  for(int i = 0; i < NUM_LEDS; i++){
    for(int ii = 0; ii < NUM_LEDS; ii++){
      leds[ii] = CRGB::Black;
    }
    leds[i] = 0x0000ff;
    FastLED.show();
    delay(25);
  }
  
  for(int i = NUM_LEDS; i > 0; i--){
    for(int ii = NUM_LEDS; ii > 0; ii--){
      leds[ii] = CRGB::Black;
    }
    leds[i] = 0x0000ff;
    FastLED.show();
    delay(25);
  }

  for(int i = 0; i < NUM_LEDS; i++){
    leds[i] = CRGB::Black;
  }
  FastLED.show();


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

  

  
  softlight.setBrightnessBase100(0);
}



void loop() {
  //if(softalarm.getOn()){
  //  softlight.setBrightness(softalarm.getBase1023());
  //  light_bright.value.int_value = softalarm.getBase100();
  //}

  softlight.compute();

  //softalarm.loop();

  if(WiFi.status() != WL_CONNECTED && millis() – lastReconnectMillis > 10000)
  {
    lastReconnectMillis = millis(); 
    WiFi.reconnect();
  }

  leds[0] = CRGB::Black;
  leds[1] = CRGB::Black;
  delay(10);
  //currentScene = detectScene();
  leds[0] = CRGB::Black;
  leds[1] = CRGB::Black;
  int frame = millis() % maxFrames;
  float offset = (float)frame / (float)maxFrames;

    leds[0] = CRGB::Black;
  leds[1] = CRGB::Black;
  //direction modifier (-1): offset * 1 OR offset * -1
  render(offset * -1, millis() - lastSceneChange);
  leds[0] = CRGB::Black;
  leds[1] = CRGB::Black;
  FastLED.show();
}


/**
 * renders LED strip. Iterates every set of LEDs, and LED per set.
 * float offset: 0-0.999... offset of period
 */
void render(float offset, int sceneFrame){
  if(sceneFrame < sceneChangeLength){
    
  }
  for(int i = 0; i <= numberOfSets; i++){
    for (int ii = 1; ii <= setSize; ii++){
      if(i * setSize + ii <= NUM_LEDS){
        float ledValueMod = ii + (setSize * offset);
        uint8_t pos = ledValueMod * (256 / setSize);
        leds[i * setSize + ii - 1] = CHSV(hue, sat, levelCalculation(pos));
      }
    }
  }
  

  
}


/**
 * Calculates actual value for LED from a defined function.
 * pos: 0-255 sweep position for LED
 */
int levelCalculation(uint8_t pos){
  //return quadwave8(pos);
  return (-0.6 * sin(6.28 * pos / 256) * (pos-256)) + 128;
  //return LUT[pos];
}


float mapf(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
