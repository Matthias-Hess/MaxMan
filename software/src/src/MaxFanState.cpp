#include "MaxFanState.h"
#include "MaxRemote.h"  // For pattern constants and temperature mappings
#include <stdlib.h>  // for strtol

// Constants and mappings are already defined in MaxRemote.h
// Use them directly - no need to redefine

// Constructor - initialize with defaults
MaxFanState::MaxFanState() {
  // Default: OFF mode with closed cover
  stateByte = STATE_OFF;
  speedByte = FAN_SPEED_20;  // Default speed
  tempFahrenheit = 68;  // Default: 20°C = 68°F
}

// Set from raw bytes (for IR reception)
void MaxFanState::SetBytes(uint8_t state, uint8_t speed, uint8_t temp) {
  stateByte = state & 0x7F;  // Mask to 7 bits
  speedByte = speed & 0x7F;
  // Convert pattern to Celsius, then to Fahrenheit
  int celsius = MaxRemote::getTemperatureFromPattern(temp & 0x7F);
  tempFahrenheit = (int8_t)((celsius * 9 / 5) + 32);  // C to F: F = (C * 9/5) + 32
}

// Set from JSON string (for BLE reception)
bool MaxFanState::SetJson(const String& jsonString) {
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, jsonString);
  
  if (error) {
    return false;
  }
  
  // Parse mode/off
  bool off = doc.containsKey("off") ? doc["off"].as<bool>() : false;
  String modeStr = doc.containsKey("mode") ? doc["mode"].as<String>() : String("");
  
  if (off || modeStr.equalsIgnoreCase("off")) {
    SetState("OFF");
  } else if (modeStr.equalsIgnoreCase("auto") || modeStr.equalsIgnoreCase("automatic")) {
    SetState("AUTO");
  } else {
    SetState("MANUAL");
  }
  
  // Parse cover/lid state
  if (doc.containsKey("lidOpen")) {
    SetCoverOpen(doc["lidOpen"].as<bool>());
  }
  
  // Parse temperature (for auto mode)
  if (doc.containsKey("temp") || doc.containsKey("temperature")) {
    int temp = doc.containsKey("temp") ? doc["temp"].as<int>() : doc["temperature"].as<int>();
    SetTempCelsius(temp);
  }
  
  // Parse speed (for manual mode)
  if (doc.containsKey("speed")) {
    int speedPercent = doc["speed"].as<int>();
    // Convert percentage (10,20,...,100) to 1-10 scale
    if (speedPercent >= 10 && speedPercent <= 100 && (speedPercent % 10 == 0)) {
      SetSpeed(speedPercent / 10);
    }
  }
  
  // Parse airIn/airFlow
  if (doc.containsKey("airIn")) {
    bool airIn = doc["airIn"].as<bool>();
    SetAirFlow(airIn ? "IN" : "OUT");
  } else if (doc.containsKey("airFlow")) {
    String airFlow = doc["airFlow"].as<String>();
    SetAirFlow(airFlow);
  }
  
  return true;
}

// Convert to JSON string (for BLE transmission)
String MaxFanState::ToJson() const {
  StaticJsonDocument<200> doc;
  
  String mode = GetState();
  doc["mode"] = mode;
  doc["off"] = (mode == "OFF");
  doc["lidOpen"] = GetCoverOpen();
  doc["temperature"] = GetTempCelsius();
  
  // Convert speed from 1-10 to percentage
  int speed1to10 = GetSpeed();
  doc["speed"] = speed1to10 * 10;
  
  // Include air flow direction
  doc["airIn"] = (GetAirFlow() == "IN");
  doc["airFlow"] = GetAirFlow();
  
  String jsonString;
  serializeJson(doc, jsonString);
  return jsonString;
}

// Get state mode: "MANUAL", "AUTO", or "OFF"
String MaxFanState::GetState() const {
  String mode;
  bool coverOpen, airIn, off;
  decodeState(mode, coverOpen, airIn, off);
  return mode;
}

// Set state mode
bool MaxFanState::SetState(const String& mode) {
  bool coverOpen = GetCoverOpen();  // Preserve current cover state
  String airFlow = GetAirFlow();    // Preserve current air flow direction
  bool airIn = (airFlow == "IN");
  
  String modeUpper = mode;
  modeUpper.toUpperCase();
  
  bool off = false;
  if (modeUpper == "OFF") {
    off = true;
    encodeState("OFF", coverOpen, airIn, off);
    return true;
  } else if (modeUpper == "AUTO" || modeUpper == "AUTOMATIC") {
    encodeState("AUTO", coverOpen, airIn, off);
    return true;
  } else if (modeUpper == "MANUAL") {
    encodeState("MANUAL", coverOpen, airIn, off);
    return true;
  }
  
  return false;  // Invalid mode
}

// Get temperature in Celsius
int MaxFanState::GetTempCelsius() const {
  // Convert Fahrenheit to Celsius: C = (F - 32) * 5/9
  return ((tempFahrenheit - 32) * 5) / 9;
}

// Set temperature in Celsius
bool MaxFanState::SetTempCelsius(int tempCelsius) {
  // Validate range: -2°C to 37°C (valid protocol range)
  if (tempCelsius < -2 || tempCelsius > 37) {
    return false;
  }
  // Convert Celsius to Fahrenheit: F = (C * 9/5) + 32
  tempFahrenheit = (int8_t)((tempCelsius * 9 / 5) + 32);
  return true;
}

// Get speed (1-10 scale)
int MaxFanState::GetSpeed() const {
  return patternToSpeed(speedByte);
}

// Set speed (1-10 scale)
bool MaxFanState::SetSpeed(int speed) {
  if (speed < 1 || speed > 10) {
    return false;
  }
  speedByte = speedToPattern(speed);
  return true;
}

// Get cover open state
bool MaxFanState::GetCoverOpen() const {
  String mode;
  bool coverOpen, airIn, off;
  decodeState(mode, coverOpen, airIn, off);
  return coverOpen;
}

// Set cover open state
void MaxFanState::SetCoverOpen(bool open) {
  String mode = GetState();
  String airFlow = GetAirFlow();
  bool airIn = (airFlow == "IN");
  bool off = (mode == "OFF");
  encodeState(mode, open, airIn, off);
}

// Get air flow direction: "IN" or "OUT"
String MaxFanState::GetAirFlow() const {
  String mode;
  bool coverOpen, airIn, off;
  decodeState(mode, coverOpen, airIn, off);
  return airIn ? "IN" : "OUT";
}

// Set air flow direction
bool MaxFanState::SetAirFlow(const String& direction) {
  String directionUpper = direction;
  directionUpper.toUpperCase();
  
  if (directionUpper != "IN" && directionUpper != "OUT") {
    return false;  // Invalid direction
  }
  
  bool airIn = (directionUpper == "IN");
  String mode = GetState();
  bool coverOpen = GetCoverOpen();
  bool off = (mode == "OFF");
  encodeState(mode, coverOpen, airIn, off);
  return true;
}

// Get temperature byte (pattern) for IR emission
uint8_t MaxFanState::GetTempByte() const {
  // Convert Fahrenheit to Celsius, then find the closest pattern
  int celsius = GetTempCelsius();
  
  // Find the closest matching pattern in the lookup table
  // Valid range is -2°C to 37°C
  if (celsius < -2) celsius = -2;
  if (celsius > 37) celsius = 37;
  
  // Look up the pattern for this Celsius value
  for (int i = 0; i < numTempMappings; i++) {
    if (temperatureMappings[i].temp == celsius) {
      return temperatureMappings[i].pattern;
    }
  }
  // Default to 20°C pattern if not found (shouldn't happen)
  return temperatureMappings[22].pattern;  // 20°C pattern
}

// Comparison operators
bool MaxFanState::operator==(const MaxFanState& other) const {
  return stateByte == other.stateByte &&
         speedByte == other.speedByte &&
         tempFahrenheit == other.tempFahrenheit;
}

bool MaxFanState::operator!=(const MaxFanState& other) const {
  return !(*this == other);
}

// Private helper: Encode state from mode and flags
void MaxFanState::encodeState(const String& mode, bool coverOpen, bool airIn, bool off) {
  if (off) {
    stateByte = STATE_OFF;
    if (coverOpen) {
      // Clear bit 3 (counting from MSB) to indicate open lid in OFF mode
      stateByte &= ~(1 << 3);
    }
  } else if (mode == "AUTO" || mode == "AUTOMATIC") {
    stateByte = STATE_AUTO;
    if (airIn) {
      stateByte |= STATE_AIR_IN;
    }
  } else {  // MANUAL
    stateByte = STATE_MANUAL;
    if (coverOpen) {
      stateByte |= STATE_OPEN;
    } else {
      stateByte |= STATE_CLOSED;
    }
    if (airIn) {
      stateByte |= STATE_AIR_IN;
    }
  }
}

// Private helper: Decode state to mode and flags
void MaxFanState::decodeState(String& mode, bool& coverOpen, bool& airIn, bool& off) const {
  uint8_t state = stateByte;
  
  // Check for OFF state (0b1111111 or 0b1110111 with lid open)
  if (state == STATE_OFF || state == 0b1110111) {
    mode = "OFF";
    off = true;
    coverOpen = (state == 0b1110111);  // Lid open if bit 3 (from MSB) is cleared
    airIn = false;
  }
  // Check for AUTO mode (0b1001011 base, or 0b1011011 with air in)
  // Mask out air direction bit (bit 5 from MSB = bit 2 from LSB)
  else if ((state & 0b1110111) == STATE_AUTO) {
    mode = "AUTO";
    off = false;
    coverOpen = false;
    airIn = (state & STATE_AIR_IN) != 0;
  }
  // Must be MANUAL mode
  else {
    mode = "MANUAL";
    off = false;
    coverOpen = (state & STATE_OPEN) != 0;
    airIn = (state & STATE_AIR_IN) != 0;
  }
}

// Private helper: Convert speed (1-10) to pattern
uint8_t MaxFanState::speedToPattern(int speed1to10) const {
  int speedPercent = speed1to10 * 10;
  switch(speedPercent) {
    case 10:   return FAN_SPEED_10;
    case 20:   return FAN_SPEED_20;
    case 30:   return FAN_SPEED_30;
    case 40:   return FAN_SPEED_40;
    case 50:   return FAN_SPEED_50;
    case 60:   return FAN_SPEED_60;
    case 70:   return FAN_SPEED_70;
    case 80:   return FAN_SPEED_80;
    case 90:   return FAN_SPEED_90;
    case 100:  return FAN_SPEED_100;
    default:   return FAN_SPEED_20;
  }
}

// Private helper: Convert pattern to speed (1-10)
int MaxFanState::patternToSpeed(uint8_t pattern) const {
  switch(pattern) {
    case FAN_SPEED_10:   return 1;
    case FAN_SPEED_20:   return 2;
    case FAN_SPEED_30:   return 3;
    case FAN_SPEED_40:   return 4;
    case FAN_SPEED_50:   return 5;
    case FAN_SPEED_60:   return 6;
    case FAN_SPEED_70:   return 7;
    case FAN_SPEED_80:   return 8;
    case FAN_SPEED_90:   return 9;
    case FAN_SPEED_100:  return 10;
    default:             return 2;  // Default to 2 (20%)
  }
}

// Temperature conversion formulas (Celsius <-> Fahrenheit)
// F = (C * 9/5) + 32
// C = (F - 32) * 5/9

// Note: Pattern conversion still uses MaxRemote's lookup table since IR protocol
// uses specific patterns that don't follow a mathematical formula.
// The formulas above are only for C<->F conversion, not for pattern conversion.

