/*
  The ESP32 request OBDII data from a car and control leds and other accessories using car's data.

  Created on 03/05/2026
*/

//LIBRARIES

//Led Strip (Addressable)
#include <Adafruit_NeoPixel.h>

//CONSTANTS

//Led Strip
#define LED_DATA_PIN 32
#define LED_COUNT    30

//GLOBAL VARIABLES
int mode = 0;
int led_helper = 0;
long time_led_helper = 0;
bool led_increase = true;

//INSTANCES

//Led Strip
Adafruit_NeoPixel strip(LED_COUNT, LED_DATA_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  
  //Led Strip
  strip.begin();
  strip.show();
  strip.setBrightness(50); //max = 255

}

void loop() {
  // put your main code here, to run repeatedly:
  switch(mode){
    case 0:
      loadingLed();
      break;
  }
}

void loadingLed(){
  if(millis() > time_led_helper + 150){
    time_led_helper = millis();
    strip.clear();
    strip.setPixelColor(led_helper, strip.Color(0,0,255));
    if(led_helper >= strip.numPixels() - 1){
      led_increase = false;
    }
    if(led_helper <= 0){
      led_increase = true;
    }
    if(led_increase){
      led_helper++;
    }else{
      led_helper--;
    }
    strip.show();
  }
}
