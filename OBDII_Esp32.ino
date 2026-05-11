/*
  The ESP32 request OBDII data from a car and control leds and other accessories using car's data.

  Created on 03/05/2026

  ELM connection got from https://forum.arduino.cc/t/esp32-with-elm327/1315461
*/

//LIBRARIES

#include <Wire.h>
#include <Arduino.h>
//Led Strip (Addressable)
#include <Adafruit_NeoPixel.h>
//obd2 / Bluetooth
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

//CONSTANTS

//Led Strip
#define LED_DATA_PIN 32
#define LED_COUNT 30
//obd2
#define ELM_MAC_ADDRESS "11:22:33:44:55:66"
// Option 1: ISO 15765-4 CAN (recommended, modern vehicles)
#define OBD_PROTOCOL "ATSP6"
#define PROTOCOL_NAME "ISO 15765-4 CAN (11bit, 500kbaud)"
// Option 2: ISO 9141-2 (older vehicles)
// #define OBD_PROTOCOL "ATSP3"
// #define PROTOCOL_NAME "ISO 9141-2"
//bluetooth
#define SERVICE_UUID "0000fff0-0000-1000-8000-00805f9b34fb"
#define CHARACTERISTIC_UUID_TX "0000fff1-0000-1000-8000-00805f9b34fb"  // ESP32 receives here
#define CHARACTERISTIC_UUID_RX "0000fff2-0000-1000-8000-00805f9b34fb"  // ESP32 sends here



//GLOBAL VARIABLES
int mode = 0;
int led_helper = 0;
long time_led_helper = 0;
bool led_increase = true;
//bluetooth
static BLEAdvertisedDevice* elmDevice = nullptr;
static BLEClient* pClient = nullptr;
static BLERemoteCharacteristic* pTxCharacteristic = nullptr;
static BLERemoteCharacteristic* pRxCharacteristic = nullptr;
static bool deviceConnected = false;
static bool doConnect = false;
static String receivedData = "";

//INSTANCES

//Led Strip
Adafruit_NeoPixel strip(LED_COUNT, LED_DATA_PIN, NEO_GRB + NEO_KHZ800);

//CALLBACKS

//Bluetooth Scan
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    // Search for ELM327 in device name (common names: "OBDII", "ELM327", "OBD2")
    String deviceName = String(advertisedDevice.getName().c_str());
    String deviceMAC = String(advertisedDevice.getAddress().toString().c_str());

    if (deviceName.indexOf("OBD") != -1 || deviceName.indexOf("ELM") != -1 || deviceName.indexOf("obd") != -1 || deviceName.indexOf("elm") != -1 || deviceMAC.indexOf("81:23:45:67:89:ba") != -1) {

      Serial.println("ELM327 device found!");
      BLEDevice::getScan()->stop();
      elmDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
    }
  }
};

//Received Data
static void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic,
                           uint8_t* pData, size_t length, bool isNotify) {
  // Convert received data to string
  for (int i = 0; i < length; i++) {
    receivedData += (char)pData[i];
  }
}

//Connection Status
class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
    Serial.println("[OK] Connected to ELM327");
    deviceConnected = true;
  }

  void onDisconnect(BLEClient* pclient) {
    Serial.println("[FAILED] Connection to ELM327 lost");
    deviceConnected = false;
  }
};

String waitForResponse(int timeoutMs = 2000) {
  unsigned long startTime = millis();

  while (millis() - startTime < timeoutMs) {
    // Check if data ends with '>' (ELM327 prompt)
    if (receivedData.indexOf('>') != -1) {
      String response = receivedData;
      receivedData = "";
      return response;
    }
    delay(10);
  }

  Serial.println("Timeout waiting for response");
  return receivedData;
}

// STANDARD FUNCTIONS

void setup() {
  //Serial
  Serial.begin(115200);

  //Led Strip
  strip.begin();
  strip.show();
  strip.setBrightness(100);  //max = 255

  // Initialize BLE
  Serial.println("Initializing BLE...");
  BLEDevice::init("ESP32_OBD_Reader");

  // Start BLE scan
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);

  Serial.println("Starting BLE scan (10 seconds)...");
  pBLEScan->start(10, false);
}

void loop() {
  // put your main code here, to run repeatedly:
  switch (mode) {
    case 0:
      //loadingLed();
      colorAll(255, 0, 0);  // RED = Failed to find ELM
      delay(2000);
      ESP.restart();
      break;
    case 1:
      if(deviceConnected){
        rpmMode1(readEngineSpeed());
      }
  }

  if (doConnect) {
    colorAll(0, 0, 255);  // BLUE = Connecting
    if (connectToELM327()) {
      doConnect = false;

      // Initialize ELM327
      if (initializeELM327()) {
        Serial.println("Ready to read!\n");
        colorAll(0, 255, 0);  // GREEN = Connected
        mode = 1;
        delay(2000);
      }
    } else {
      doConnect = false;
      colorAll(255, 0, 0);  // RED = Failed to connect
      Serial.println("Connection failed. Restarting in 5 seconds...");
      delay(5000);
      ESP.restart();
    }
  }

  // If connected, read temperature every 5 seconds
  if (deviceConnected) {
    // Run a test on first iteration
    static bool firstRun = true;
    if (firstRun) {
      Serial.println("\n>>> Running test query <<<");

      // Test: Query supported PIDs (0100)
      sendOBDCommand("0100");
      delay(1000);
      String testResponse = waitForResponse(3000);
      Serial.print("Test response (0100): ");
      Serial.println(testResponse);

      delay(1000);
      firstRun = false;
    }
    readCoolantTemperature();
    delay(100);
  }

  static bool wasConnected = false;
    if (!deviceConnected && wasConnected) {
        Serial.println("Connection lost! Trying to reconnect...");
        
        delay(5000);
        ESP.restart();
    }
    
    // Save status
    if (deviceConnected) {
        wasConnected = true;
    }

}

// ELM327 FUNCTIONS

bool connectToELM327() {
  Serial.println("\nTrying to connect to device...");

  // Create BLE client
  pClient = BLEDevice::createClient();
  Serial.println("[OK] BLE client created");

  // Set callback for connection status
  pClient->setClientCallbacks(new MyClientCallback());

  // Connect to ELM327
  Serial.print("Connecting to ");
  Serial.println(elmDevice->getAddress().toString().c_str());

  if (!pClient->connect(elmDevice)) {
    Serial.println("[FAILED] Connection failed");
    return false;
  }
  Serial.println("[OK] Connection established");

  Serial.println("\nChecking the connection properties...");

  // Retrieve service
  BLERemoteService* pRemoteService = pClient->getService(SERVICE_UUID);
  if (pRemoteService == nullptr) {
    Serial.println("[FAILED] Service not found");
    pClient->disconnect();
    return false;
  }
  Serial.println("[OK] Service found");

  // Retrieve TX characteristic (ESP32 receives)
  pTxCharacteristic = pRemoteService->getCharacteristic(CHARACTERISTIC_UUID_TX);
  if (pTxCharacteristic == nullptr) {
    Serial.println("[FAILED] TX characteristic not found");
    pClient->disconnect();
    return false;
  }
  Serial.println("[OK] TX characteristic found");

  // Retrieve RX characteristic (ESP32 sends)
  pRxCharacteristic = pRemoteService->getCharacteristic(CHARACTERISTIC_UUID_RX);
  if (pRxCharacteristic == nullptr) {
    Serial.println("[FAILED] RX characteristic not found");
    pClient->disconnect();
    return false;
  }
  Serial.println("[OK] RX characteristic found");

  // Enable notifications for incoming data
  if (pTxCharacteristic->canNotify()) {
    pTxCharacteristic->registerForNotify(notifyCallback);
    Serial.println("[OK] Notifications enabled");
  }

  Serial.println("The connection with ELM327 is OK\n");
  return true;
}

void sendOBDCommand(String command) {
  if (!deviceConnected) {
    Serial.println("[FAILED] Not connected - command will not be sent");
    return;
  }

  receivedData = "";  // Clear receive buffer
  command += "\r";    // Append carriage return (ELM327 expects this)

  pRxCharacteristic->writeValue(command.c_str(), command.length());
}

bool initializeELM327() {
  Serial.println("\nTrying to setup the ELM327...");

  // Reset
  sendOBDCommand("ATZ");
  delay(2500);  // Extra time for complete reset
  String response = waitForResponse(3000);
  Serial.println(response);

  // Wait until ELM327 is really ready
  delay(500);

  // Turn off echo (for cleaner responses)
  sendOBDCommand("ATE0");
  delay(500);
  response = waitForResponse();
  Serial.print("Echo off: ");
  Serial.println(response);

  // Remove spaces from responses
  sendOBDCommand("ATS0");
  delay(500);
  response = waitForResponse();
  Serial.print("Spaces off: ");
  Serial.println(response);

  // Turn off linefeed
  sendOBDCommand("ATL0");
  delay(500);
  response = waitForResponse();

  // IMPORTANT: Set protocol MANUALLY instead of auto-detection
  Serial.print("Setting protocol to ");
  Serial.print(PROTOCOL_NAME);
  Serial.println("...");

  sendOBDCommand(OBD_PROTOCOL);
  delay(2000);  // Longer wait time for protocol setup
  response = waitForResponse(2500);
  Serial.print("Protocol set: ");
  Serial.println(response);

  // Turn off headers (can help with some simulators)
  sendOBDCommand("ATH0");
  delay(500);
  response = waitForResponse();
  Serial.print("Headers off: ");
  Serial.println(response);

  // Set adaptive timing to auto (IMPORTANT for ISO14230-4!)
  sendOBDCommand("ATAT2");
  delay(500);
  response = waitForResponse();
  Serial.print("Adaptive timing: ");
  Serial.println(response);

  // Set timeout to a higher value for ISO14230-4
  // ATST FF = 255 × 4ms = 1020ms timeout
  sendOBDCommand("ATSTFF");
  delay(500);
  response = waitForResponse();
  Serial.print("Timeout set: ");
  Serial.println(response);

  // Allow long messages (important for some protocols)
  sendOBDCommand("ATAL");
  delay(500);
  response = waitForResponse();
  Serial.print("Long messages: ");
  Serial.println(response);

  // Enable keep alive - prevents connection drop!
  // Automatically sends small packets to maintain connection
  sendOBDCommand("ATKW");
  delay(500);
  response = waitForResponse();
  Serial.print("Keep Alive Status: ");
  Serial.println(response);

  // Query used protocol
  sendOBDCommand("ATDPN");
  delay(500);
  response = waitForResponse();
  Serial.print(">>> Used protocol number: ");
  Serial.println(response);

  sendOBDCommand("ATDP");
  delay(500);
  response = waitForResponse();
  Serial.print(">>> Protocol name: ");
  Serial.println(response);

  // Voltage test (checks if OBD connection is established)
  sendOBDCommand("ATRV");
  delay(500);
  response = waitForResponse();
  Serial.print("Voltage: ");
  Serial.println(response);

  // IMPORTANT: Initial "dummy" bus initialization
  // This ensures the bus is really connected
  Serial.println("\n>>> Performing initial bus connection <<<");
  sendOBDCommand("0100");
  delay(1500);
  response = waitForResponse(5000);  // Long timeout for first connection
  Serial.print("Initial bus response: ");
  Serial.println(response);

  // If BUS INIT ERROR, try again
  if (response.indexOf("ERROR") != -1 || response.indexOf("UNABLE") != -1) {
    Serial.println("Bus init failed, retrying...");
    delay(1000);

    // Set protocol again
    sendOBDCommand(OBD_PROTOCOL);
    delay(2000);
    waitForResponse(2500);

    // Try again
    sendOBDCommand("0100");
    delay(1500);
    response = waitForResponse(5000);
    Serial.print("Second attempt: ");
    Serial.println(response);
  }

  Serial.println("[OK] ELM327 initialized");
  Serial.println("=== Initialization complete ===\n");

  // Short pause before normal queries start
  delay(1000);

  return true;
}

//READ OBD FUNCTIONS

float readCoolantTemperature() {
  // OBD2 Mode 01 PID 05: Engine Coolant Temperature
  sendOBDCommand("0105");
  delay(300);  // Longer wait for ISO14230-4

  String response = waitForResponse(4000);  // Longer timeout for ISO14230-4

  // Check if an error was returned
  if (response.indexOf("NO DATA") != -1) {
    Serial.println("[ERROR] No data - reinitializing bus...");

    // Complete bus reinitialization
    sendOBDCommand("ATZ");
    delay(2000);
    waitForResponse(3000);

    // Set protocol again
    sendOBDCommand(OBD_PROTOCOL);
    delay(1500);
    waitForResponse(2000);

    // Adaptive timing
    sendOBDCommand("ATAT2");
    delay(500);
    waitForResponse();

    Serial.println("Bus reinitialized - retrying...");

    // Try again
    sendOBDCommand("0105");
    delay(800);
    response = waitForResponse(4000);

    Serial.print("New response after reset: [");
    Serial.print(response);
    Serial.println("]");

    // If still NO DATA, give up
    if (response.indexOf("NO DATA") != -1 || response.indexOf("UNABLE") != -1) {
      Serial.println("[ERROR] Still no data after reset");
      return -999.0;
    }
  }

  if (response.indexOf("UNABLE TO CONNECT") != -1) {
    Serial.println("[FAILED] Connection to OBD bus failed - attempting reconnect");

    // Similar reset as for NO DATA
    sendOBDCommand("ATZ");
    delay(2000);
    waitForResponse(3000);

    sendOBDCommand(OBD_PROTOCOL);
    delay(1500);
    waitForResponse(2000);

    return -999.0;
  }

  if (response.indexOf("BUS INIT") != -1 && response.indexOf("ERROR") != -1) {
    Serial.println("[ERROR] Bus initialization failed");
    return -999.0;
  }

  if (response.indexOf("?") != -1) {
    Serial.println("[ERROR] Unknown command");
    return -999.0;
  }

  // Parse response: format is "41 05 XX" where XX is the hex value
  // Temperature = XX - 40 (in °C)

  // Remove spaces for easier parsing
  response.replace(" ", "");
  response.replace("\r", "");
  response.replace("\n", "");
  response.replace(">", "");

  int pidPos = response.indexOf("4105");
  if (pidPos == -1) {
    Serial.println("[ERROR] Invalid response or PID not supported");
    Serial.print("Cleaned response was: ");
    Serial.println(response);
    return -999.0;
  }

  // Extract hex value after "4105" (2 characters)
  String hexValue = response.substring(pidPos + 4, pidPos + 6);
  hexValue.trim();

  if (hexValue.length() < 2) {
    Serial.println("[ERROR] Hex value too short");
    return -999.0;
  }

  // Convert hex to decimal
  int tempValue = (int)strtol(hexValue.c_str(), NULL, 16);

  // Calculate temperature (formula: value - 40)
  float temperature = tempValue - 40.0;

  Serial.print(temperature);
  Serial.println(" °C");

  return temperature;
}

float readEngineSpeed() {
  // OBD2 Mode 01 PID 0C: Engine Speed (RPM)
  sendOBDCommand("010C");
  delay(300);  // Longer wait for ISO14230-4

  String response = waitForResponse(4000);  // Longer timeout for ISO14230-4

  // Check if an error was returned
  if (response.indexOf("NO DATA") != -1) {
    Serial.println("[ERROR] No data - reinitializing bus...");

    // Complete bus reinitialization
    sendOBDCommand("ATZ");
    delay(2000);
    waitForResponse(3000);

    // Set protocol again
    sendOBDCommand(OBD_PROTOCOL);
    delay(1500);
    waitForResponse(2000);

    // Adaptive timing
    sendOBDCommand("ATAT2");
    delay(500);
    waitForResponse();

    Serial.println("Bus reinitialized - retrying...");

    // Try again
    sendOBDCommand("010C");
    delay(800);
    response = waitForResponse(4000);

    Serial.print("New response after reset: [");
    Serial.print(response);
    Serial.println("]");

    // If still NO DATA, give up
    if (response.indexOf("NO DATA") != -1 || response.indexOf("UNABLE") != -1) {
      Serial.println("[ERROR] Still no data after reset");
      return -999.0;
    }
  }

  if (response.indexOf("UNABLE TO CONNECT") != -1) {
    Serial.println("[FAILED] Connection to OBD bus failed - attempting reconnect");

    // Similar reset as for NO DATA
    sendOBDCommand("ATZ");
    delay(2000);
    waitForResponse(3000);

    sendOBDCommand(OBD_PROTOCOL);
    delay(1500);
    waitForResponse(2000);

    return -999.0;
  }

  if (response.indexOf("BUS INIT") != -1 && response.indexOf("ERROR") != -1) {
    Serial.println("[ERROR] Bus initialization failed");
    return -999.0;
  }

  if (response.indexOf("?") != -1) {
    Serial.println("[ERROR] Unknown command");
    return -999.0;
  }

  // Parse response: format is "41 0C XX YY" where XX YY is the hex value
  // RPM = ((XX * 256) + YY) / 4

  // Remove spaces for easier parsing
  response.replace(" ", "");
  response.replace("\r", "");
  response.replace("\n", "");
  response.replace(">", "");

  int pidPos = response.indexOf("410C");
  if (pidPos == -1) {
    Serial.println("[ERROR] Invalid response or PID not supported");
    Serial.print("Cleaned response was: ");
    Serial.println(response);
    return -999.0;
  }

  // Extract two hex bytes after "410C" (4 characters total)
  String hexA = response.substring(pidPos + 4, pidPos + 6);
  String hexB = response.substring(pidPos + 6, pidPos + 8);
  hexA.trim();
  hexB.trim();

  if (hexA.length() < 2 || hexB.length() < 2) {
    Serial.println("[ERROR] Hex value too short");
    return -999.0;
  }

  // Convert hex bytes to decimal
  int byteA = (int)strtol(hexA.c_str(), NULL, 16);
  int byteB = (int)strtol(hexB.c_str(), NULL, 16);

  // Calculate RPM (formula: ((A * 256) + B) / 4)
  float rpm = ((byteA * 256) + byteB) / 4.0;

  Serial.print(rpm);
  Serial.println(" RPM");

  return rpm;
}

// LED FUNCTIONS

void loadingLed() {
  if (millis() > time_led_helper + 150) {
    time_led_helper = millis();
    strip.clear();
    strip.setPixelColor(led_helper, strip.Color(0, 0, 255));
    if (led_helper >= strip.numPixels() - 1) {
      led_increase = false;
    }
    if (led_helper <= 0) {
      led_increase = true;
    }
    if (led_increase) {
      led_helper++;
    } else {
      led_helper--;
    }
    strip.show();
  }
}

void colorAll(int r, int g, int b) {
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(r, g, b));
  }
  strip.show();
}

void rpmMode1(float rpm){
  int ledValue = (int)map(rpm, 0, 8000, 0, strip.numPixels());
  strip.clear();
  for (int i = 0; i < ledValue; i++) {
    strip.setPixelColor(i, strip.Color(217, 142, 2));
  }
  strip.show();

}