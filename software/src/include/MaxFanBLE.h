#ifndef MAXFANBLE_H
#define MAXFANBLE_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <ArduinoJson.h>

// BLE Service UUID
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
// BLE Characteristic UUIDs
#define COMMAND_UUID        "beb5483e-36e1-4688-b7f5-ea07361b26a8"  // Write: Send commands
#define STATUS_UUID         "cba1d466-344c-4be3-ab3f-1890d5c0c0c0"  // Read/Notify: Current status

// Structure to hold fan state
struct FanState {
  String mode;        // "auto", "manual", or "off"
  int temperature;    // -2 to 37 (for auto mode)
  int speed;          // 10, 20, 30, ..., 100 (for manual mode)
  bool lidOpen;       // true/false
  bool airIn;         // true/false
  bool off;           // true/false
};

class MaxFanBLE {
public:
  // Callback function type for when a command is received
  typedef void (*CommandCallback)(const FanState& state);
  
  MaxFanBLE();
  ~MaxFanBLE();
  
  // Initialize BLE
  void begin(const char* deviceName = "MaxxFan Controller");
  
  // Set callback for when commands are received
  void setCommandCallback(CommandCallback callback);
  
  // Check if a client is connected
  bool isConnected();
  
  // Get the last known state
  FanState getState();
  
  // Set state (called when IR command is sent or received)
  void setState(const FanState& state);
  
  // Notify connected clients of state change
  void notifyStateChange();
  
  // Process incoming BLE command
  // Returns true if command was successfully parsed and should be sent
  bool processCommand(const String& jsonCommand, FanState& outState);

private:
  BLEServer* pServer;
  BLEService* pService;
  BLECharacteristic* pCommandCharacteristic;
  BLECharacteristic* pStatusCharacteristic;
  
  bool deviceConnected;
  bool oldDeviceConnected;
  
  FanState currentState;
  CommandCallback commandCallback;
  
  // BLE callbacks
  class ServerCallbacks: public BLEServerCallbacks {
    MaxFanBLE* parent;
  public:
    ServerCallbacks(MaxFanBLE* p) : parent(p) {}
    
    void onConnect(BLEServer* pServer) {
      parent->deviceConnected = true;
    }
    
    void onDisconnect(BLEServer* pServer) {
      parent->deviceConnected = false;
    }
  };
  
  class CommandCallbacks: public BLECharacteristicCallbacks {
    MaxFanBLE* parent;
  public:
    CommandCallbacks(MaxFanBLE* p) : parent(p) {}
    
    void onWrite(BLECharacteristic* pCharacteristic) {
      std::string value = pCharacteristic->getValue();
      if (value.length() > 0) {
        String jsonCommand = String(value.c_str());
        FanState newState;
        if (parent->processCommand(jsonCommand, newState)) {
          // Command parsed successfully, update state and trigger callback
          parent->setState(newState);
          if (parent->commandCallback) {
            parent->commandCallback(newState);
          }
        }
      }
    }
  };
};

#endif // MAXFANBLE_H

