# Control leds with car's data

You can use an ESP32 to read the car data (using a OBDII Scanner) and control a led strip and a 2-digit 7segment display.

## Overview

### The OBD II Communication

This code dont read the car data directly. It connects with a bluetooth module (ELM-327) that is connected on your car's OBD port. The ELM-327 allows the ESP send and receive OBD data by commands, through Bluetooth Serial.

### The Led Strip

The code consider an Addressable RGB Led Strip, that use 3 pins (5V, Data, GND). The electrical circuit have to care about the current (A), because the leds can easily use more current than supported by ESP, so consider not to connect the led strip 5v on ESP pin. You can test the worst case turning on all the leds with max brightness and white color.

### The 2 Digit Display

For this project, two 7 segment display is used, with a home made electrical circuit to regulate the energy used to power and control the displays.

### Access Point

When the code initialize, a Access Point is created. When a device is connected to it, the 192.168.0.1 address will be a web page to control things on ESP, like led mode.

## How to Use

### Preparing the code

Verify your ELM-327 MAC Address and change the ELM_MAC_ADDRESS constant with it. Change the LED_DATA_PIN with the data pin of the led strip and the LED_COUNT with the number of leds in your led strip.
The OBD_PROTOCOL and PROTOCOL_NAME already setted should work fine, but in some cars or simulators it may change.
You can change the ssid and password constants with what you want, these will be the Access Point credentials. Change the Displays pins according your project, the pins are in alphabetical order (A, B, C, D, E, F, G) and each one controls a segment of the display.

### Setting up

Connect the ELM-327 on OBD port and turn on the vehicle. Turn on the ESP and wait to connect. For some reason, the ESP may not connect, so restart the ESP and try again.
