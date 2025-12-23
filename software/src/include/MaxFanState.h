#ifndef MAXFANSTATE_H
#define MAXFANSTATE_H

#include <Arduino.h>
#include <ArduinoJson.h>

enum class MaxFanMode { OFF, AUTO, MANUAL };
enum class MaxFanDirection { IN, OUT };

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
  MaxFanMode GetMode() const;  // Returns "MANUAL", "AUTO", or "OFF"
  void SetMode(MaxFanMode mode);
  
  // Temperature accessors (for AUTO mode) - works with Celsius
  // Note: Internal storage uses protocol pattern, conversion via lookup table
  int GetTempCelsius() const;
  bool SetTempCelsius(int tempCelsius);  // Returns true if valid temp set
  
  
  int GetSpeed() const;  // Returns 10-100
  void SetSpeed(int speed);  // Valid range: 10-100
  
  // Cover/lid state accessors
  bool GetCoverOpen() const;
  void SetCoverOpen(bool open);
  
  // Air flow direction accessors
  MaxFanDirection GetAirFlow() const;  
  void SetAirFlow(MaxFanDirection direction);  // Returns true if valid direction set
  
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
  uint8_t speedByte;  // speed (10, 20, 30, .. 100)
  uint8_t tempFahrenheit;  // Temperature in Fahrenheit
  
  
  
  
  
};

#endif // MAXFANSTATE_H

