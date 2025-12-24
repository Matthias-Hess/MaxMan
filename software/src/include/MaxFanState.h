#ifndef MAXFANSTATE_H
#define MAXFANSTATE_H

#include <Arduino.h>
#include <ArduinoJson.h>

enum class CoverState {CLOSED, OPEN};
std::string toString(CoverState mode);
CoverState toCoverState(const std::string& str);

enum class MaxFanMode { OFF, AUTO, MANUAL };
std::string toString(MaxFanMode mode);
MaxFanMode toMaxFanMode(const std::string& str);


enum class MaxFanDirection { IN, OUT };
std::string toString(MaxFanDirection mode);
MaxFanDirection toMaxFanDirection(const std::string& str);

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
  MaxFanMode GetMode() const;  
  void SetMode(MaxFanMode mode);
  

  int GetTempCelsius() const;
  bool SetTempCelsius(int tempCelsius);  
  
  
  int GetSpeed() const;  
  void SetSpeed(int speed);  
  

  CoverState GetCover() const;
  void SetCover(CoverState);
  

  MaxFanDirection GetAirFlow() const;  
  void SetAirFlow(MaxFanDirection);  // Returns true if valid direction set
  

  uint8_t GetStateByte() const { return stateByte; }
  uint8_t GetSpeedByte() const { return speedByte; }
  uint8_t GetTempByte() const  { return tempFahrenheit;}  



  bool operator==(const MaxFanState& other) const;
  bool operator!=(const MaxFanState& other) const;
  
private:
  // Core storage
  uint8_t stateByte;  // 7-bit state pattern
  uint8_t speedByte;  // speed (10, 20, 30, .. 100)
  uint8_t tempFahrenheit;  // Temperature in Fahrenheit
};

#endif // MAXFANSTATE_H

