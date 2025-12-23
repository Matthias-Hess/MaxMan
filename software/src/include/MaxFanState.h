#ifndef MAXFANSTATE_H
#define MAXFANSTATE_H

#include <Arduino.h>
#include <ArduinoJson.h>

// Canonical command/state representation for MaxxFan
// Core: 3 bytes (state, speed, temp) stored as 7-bit patterns
class MaxFanState {
public:
  MaxFanState();
  
  // Initialize from raw bytes (for IR reception)
  void SetBytes(uint8_t state, uint8_t speed, uint8_t temp);
  
  // Initialize from JSON string (for BLE reception)
  bool SetJson(const String& jsonString);
  
  // Convert to JSON string (for BLE transmission)
  String ToJson() const;
  
  // State mode accessors
  String GetState() const;  // Returns "MANUAL", "AUTO", or "OFF"
  bool SetState(const String& mode);  // Returns true if valid mode set
  
  // Temperature accessors (for AUTO mode) - works with Celsius
  // Note: Internal storage uses protocol pattern, conversion via lookup table
  int GetTempCelsius() const;
  bool SetTempCelsius(int tempCelsius);  // Returns true if valid temp set
  
  // Speed accessors (for MANUAL mode) - 1-10 scale (1=10%, 2=20%, ..., 10=100%)
  int GetSpeed() const;  // Returns 1-10
  bool SetSpeed(int speed);  // Valid range: 1-10, returns true if valid
  
  // Cover/lid state accessors
  bool GetCoverOpen() const;
  void SetCoverOpen(bool open);
  
  // Air flow direction accessors
  String GetAirFlow() const;  // Returns "IN" or "OUT"
  bool SetAirFlow(const String& direction);  // Returns true if valid direction set
  
  // Direct byte accessors (for IR emission)
  uint8_t GetStateByte() const { return stateByte; }
  uint8_t GetSpeedByte() const { return speedByte; }
  uint8_t GetTempByte() const;  // Converts Fahrenheit to pattern for IR emission
  
  // Comparison operators
  bool operator==(const MaxFanState& other) const;
  bool operator!=(const MaxFanState& other) const;
  
private:
  // Core storage
  uint8_t stateByte;  // 7-bit state pattern
  uint8_t speedByte;  // 7-bit speed pattern
  int8_t tempFahrenheit;  // Temperature in Fahrenheit (signed: -4 to 99 covers -2°C to 37°C)
  
  // Helper methods for encoding/decoding
  void encodeState(const String& mode, bool coverOpen, bool airIn, bool off);
  void decodeState(String& mode, bool& coverOpen, bool& airIn, bool& off) const;
  uint8_t speedToPattern(int speed1to10) const;  // Convert 1-10 to pattern
  int patternToSpeed(uint8_t pattern) const;      // Convert pattern to 1-10
};

#endif // MAXFANSTATE_H

