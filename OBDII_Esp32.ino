/*
  The ESP32 request OBDII data from a car and control leds and other accessories using car's data.

  Created on 03/05/2026
*/

//LIBRARIES

//Led Strip (Addressable)
#include <Adafruit_NeoPixel.h>
//obd2
#include <Arduino.h>
#include "BluetoothSerial.h"

//CONSTANTS

//Led Strip
#define LED_DATA_PIN 32
#define LED_COUNT    30
//obd2
#define BT_NAME "ESP32"
uint8_t elmMAC[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

//GLOBAL VARIABLES
int mode = 0;
int led_helper = 0;
long time_led_helper = 0;
bool led_increase = true;

bool elm_ready = false;

//INSTANCES

//Led Strip
Adafruit_NeoPixel strip(LED_COUNT, LED_DATA_PIN, NEO_GRB + NEO_KHZ800);
//Bluetooth
BluetoothSerial SerialBT;

String sendCommand(const char* cmd, int timeout = 1000) {
  SerialBT.println(cmd);
  String response = "";
  unsigned long start = millis();
  while (millis() - start < timeout) {
    while (SerialBT.available()) {
      char c = SerialBT.read();
      response += c;
    }
    if (response.indexOf('>') >= 0) break;
  }
  response.trim();
  return response;
}

void setup() {
  //Serial
  Serial.begin(115200);
  Serial.println("Starting Bluetooth");
  SerialBT.begin(BT_NAME, true);

  Serial.println("Searching OBD reader device (elm)...");
  if(SerialBT.connect(elmMAC)){
    Serial.println("Connected to device by Bluetooth!");
    startELM();
  }else{
    Serial.println("Failed to connect.");
  }
  
  //Led Strip
  strip.begin();
  strip.show();
  strip.setBrightness(50); //max = 255

}

void loop() {
  // put your main code here, to run repeatedly:
  switch(mode){
    case 0: // Only loading
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

void startELM(){
  delay(1000);
  sendCommand("ATE0"); // Echo off
  sendCommand("ATL0"); // Linefeeds off
  sendCommand("ATH0"); // Headers off
  elm_ready = true;
  Serial.println("ELM327 ready!");
}

