#include "MaxFanBLE.h"

MaxFanBLE::MaxFanBLE() 
  : pServer(nullptr), pService(nullptr), 
    pCommandCharacteristic(nullptr), pStatusCharacteristic(nullptr),
    deviceConnected(false), oldDeviceConnected(false),
    commandCallback(nullptr) {
  // Initialize default state
  currentState.mode = "off";
  currentState.temperature = 20;
  currentState.speed = 20;
  currentState.lidOpen = false;
  currentState.airIn = false;
  currentState.off = true;
}

MaxFanBLE::~MaxFanBLE() {
  // Cleanup handled by BLE stack
}

void MaxFanBLE::setCommandCallback(CommandCallback callback) {
  commandCallback = callback;
}

void MaxFanBLE::begin(const char* deviceName) {
  // Initialize BLE
  BLEDevice::init(deviceName);
  
  // Create BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks(this));
  
  // Create BLE Service
  pService = pServer->createService(SERVICE_UUID);
  
  // Create Command Characteristic (Write only)
  pCommandCharacteristic = pService->createCharacteristic(
    COMMAND_UUID,
    BLECharacteristic::PROPERTY_WRITE
  );
  pCommandCharacteristic->setCallbacks(new CommandCallbacks(this));
  
  // Create Status Characteristic (Read + Notify)
  pStatusCharacteristic = pService->createCharacteristic(
    STATUS_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
  );
  pStatusCharacteristic->addDescriptor(new BLE2902());
  
  // Start the service
  pService->start();
  
  // Start advertising
  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  
  Serial.println("BLE Server started, waiting for connections...");
}

bool MaxFanBLE::isConnected() {
  return deviceConnected;
}

FanState MaxFanBLE::getState() {
  return currentState;
}

void MaxFanBLE::setState(const FanState& state) {
  currentState = state;
  notifyStateChange();
}

void MaxFanBLE::notifyStateChange() {
  if (deviceConnected && pStatusCharacteristic) {
    // Convert state to JSON
    StaticJsonDocument<200> doc;
    doc["mode"] = currentState.mode;
    doc["temperature"] = currentState.temperature;
    doc["speed"] = currentState.speed;
    doc["lidOpen"] = currentState.lidOpen;
    doc["airIn"] = currentState.airIn;
    doc["off"] = currentState.off;
    
    String jsonString;
    serializeJson(doc, jsonString);
    
    pStatusCharacteristic->setValue(jsonString.c_str());
    pStatusCharacteristic->notify();
  }
}

bool MaxFanBLE::processCommand(const String& jsonCommand, FanState& outState) {
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, jsonCommand);
  
  if (error) {
    Serial.print("JSON parse error: ");
    Serial.println(error.c_str());
    return false;
  }
  
  // Initialize with current state
  outState = currentState;
  
  // Parse mode
  if (doc.containsKey("mode")) {
    const char* modeStr = doc["mode"];
    outState.mode = String(modeStr);
  }
  
  // Parse temperature (for auto mode)
  if (doc.containsKey("temp") || doc.containsKey("temperature")) {
    int temp = doc.containsKey("temp") ? doc["temp"] : doc["temperature"];
    if (temp >= -2 && temp <= 37) {
      outState.temperature = temp;
    }
  }
  
  // Parse speed (for manual mode)
  if (doc.containsKey("speed")) {
    int speed = doc["speed"];
    // Validate speed is one of the allowed values
    if (speed == 10 || speed == 20 || speed == 30 || speed == 40 || 
        speed == 50 || speed == 60 || speed == 70 || speed == 80 || 
        speed == 90 || speed == 100) {
      outState.speed = speed;
    }
  }
  
  // Parse lidOpen
  if (doc.containsKey("lidOpen")) {
    outState.lidOpen = doc["lidOpen"];
  }
  
  // Parse airIn
  if (doc.containsKey("airIn")) {
    outState.airIn = doc["airIn"];
  }
  
  // Parse off
  if (doc.containsKey("off")) {
    outState.off = doc["off"];
  }
  
  // Determine mode based on off flag or explicit mode
  if (outState.off) {
    outState.mode = "off";
  } else if (outState.mode == "auto" || outState.mode == "automatic") {
    outState.mode = "auto";
    outState.off = false;
  } else {
    outState.mode = "manual";
    outState.off = false;
  }
  
  Serial.print("Parsed BLE command: mode=");
  Serial.print(outState.mode);
  Serial.print(", temp=");
  Serial.print(outState.temperature);
  Serial.print(", speed=");
  Serial.print(outState.speed);
  Serial.print(", lidOpen=");
  Serial.print(outState.lidOpen);
  Serial.print(", airIn=");
  Serial.print(outState.airIn);
  Serial.print(", off=");
  Serial.println(outState.off);
  
  return true;
}

