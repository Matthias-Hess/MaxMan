#include "FanStateConverter.h"
#include "MaxRemote.h"
#include "MaxReceiver.h"
#include <stdlib.h>  // for strtol

// Forward declarations for decode helpers (defined in MaxReceiver.cpp)
extern int binaryStringToInt(const String &binStr);
extern int decodeSpeed(const String &speedStr);
extern int decodeTemperature(const String &tempStr);

// Helper: Convert uint8_t to 7-bit binary string (MSB first)
// Note: This duplicates MaxRemote::uint7ToBinaryString but it's private there
static String uint7ToBinaryString(uint8_t val) {
  String s = "";
  for (int i = 6; i >= 0; i--) {
    s += ((val >> i) & 1) ? "1" : "0";
  }
  return s;
}

// Convert FanState to MaxFanCommand (reverse of commandToFanState)
MaxFanCommand fanStateToCommand(const FanState& state) {
  MaxFanCommand cmd;
  
  // Use constants from MaxRemote.h (they're static const, so accessible)
  // We need to access them, but they're in MaxRemote.cpp's translation unit
  // So we'll redefine them locally (same values)
  const uint8_t STATE_AUTO   = 0b1001011;
  const uint8_t STATE_MANUAL = 0b0000111;
  const uint8_t STATE_OPEN   = 0b0100000;
  const uint8_t STATE_AIR_IN = 0b0010000;
  const uint8_t STATE_CLOSED = 0b0001000;
  const uint8_t STATE_OFF    = 0b1111111;
  
  uint8_t stateBits = 0;
  
  if (state.off) {
    // Off mode
    stateBits = STATE_OFF;
    if (state.lidOpen) {
      // Clear bit 3 (counting from left, bit index 3 from MSB)
      stateBits &= ~(1 << 3);
    }
  } else if (state.mode == "auto" || state.mode == "automatic") {
    // Automatic mode
    stateBits = STATE_AUTO;
    if (state.airIn) {
      stateBits |= STATE_AIR_IN;
    }
  } else {
    // Manual mode
    stateBits = STATE_MANUAL;
    if (state.lidOpen) {
      stateBits |= STATE_OPEN;
    } else {
      stateBits |= STATE_CLOSED;
    }
    if (state.airIn) {
      stateBits |= STATE_AIR_IN;
    }
  }
  cmd.state = uint7ToBinaryString(stateBits);
  
  // Fan speed patterns
  const uint8_t FAN_SPEED_10   = 0b1010111;
  const uint8_t FAN_SPEED_20   = 0b1101011;
  const uint8_t FAN_SPEED_30   = 0b1000011;
  const uint8_t FAN_SPEED_40   = 0b1110101;
  const uint8_t FAN_SPEED_50   = 0b1011001;
  const uint8_t FAN_SPEED_60   = 0b1100001;
  const uint8_t FAN_SPEED_70   = 0b1001110;
  const uint8_t FAN_SPEED_80   = 0b1111010;
  const uint8_t FAN_SPEED_90   = 0b1010010;
  const uint8_t FAN_SPEED_100  = 0b1101100;
  
  uint8_t speedBits = FAN_SPEED_20; // default
  switch(state.speed) {
    case 10:  speedBits = FAN_SPEED_10; break;
    case 20:  speedBits = FAN_SPEED_20; break;
    case 30:  speedBits = FAN_SPEED_30; break;
    case 40:  speedBits = FAN_SPEED_40; break;
    case 50:  speedBits = FAN_SPEED_50; break;
    case 60:  speedBits = FAN_SPEED_60; break;
    case 70:  speedBits = FAN_SPEED_70; break;
    case 80:  speedBits = FAN_SPEED_80; break;
    case 90:  speedBits = FAN_SPEED_90; break;
    case 100: speedBits = FAN_SPEED_100; break;
    default:  speedBits = FAN_SPEED_20; break;
  }
  cmd.speed = uint7ToBinaryString(speedBits);
  
  // Temperature mappings (same as in MaxRemote.h)
  struct TempMapping {
    int temp;
    uint8_t pattern;
  };
  static const TempMapping tempMappings[] = {
    { -2, 0b0100011 }, { -1, 0b0000011 }, {  0, 0b1111101 }, {  1, 0b0111101 },
    {  2, 0b0011101 }, {  3, 0b0101101 }, {  4, 0b0001101 }, {  5, 0b0110101 },
    {  6, 0b1010101 }, {  7, 0b1100101 }, {  8, 0b1000101 }, {  9, 0b1111001 },
    { 10, 0b1011001 }, { 11, 0b0011001 }, { 12, 0b0101001 }, { 13, 0b0001001 },
    { 14, 0b0110001 }, { 15, 0b0010001 }, { 16, 0b1100001 }, { 17, 0b1000001 },
    { 18, 0b1111110 }, { 19, 0b1011110 }, { 20, 0b1101110 }, { 21, 0b0101110 },
    { 22, 0b0001110 }, { 23, 0b0110110 }, { 24, 0b0010110 }, { 25, 0b0100110 },
    { 26, 0b1000110 }, { 27, 0b1111010 }, { 28, 0b1011010 }, { 29, 0b1101010 },
    { 30, 0b1001010 }, { 31, 0b0001010 }, { 32, 0b0110010 }, { 33, 0b0010010 },
    { 34, 0b0100010 }, { 35, 0b0000010 }, { 36, 0b1111100 }, { 37, 0b1011100 }
  };
  static const int numTempMappings = sizeof(tempMappings) / sizeof(tempMappings[0]);
  
  uint8_t tempBits = 0b1101110; // default (20°C)
  for (int i = 0; i < numTempMappings; i++) {
    if (tempMappings[i].temp == state.temperature) {
      tempBits = tempMappings[i].pattern;
      break;
    }
  }
  cmd.temp = uint7ToBinaryString(tempBits);
  
  return cmd;
}

// Convert MaxFanCommand to FanState
FanState commandToFanState(const MaxFanCommand& cmd) {
  FanState state;
  
  // Decode state bits
  int stateVal = binaryStringToInt(cmd.state);
  
  // Check for OFF state
  if (stateVal == 0b1111111) {
    state.mode = "off";
    state.off = true;
    state.lidOpen = false;
    state.airIn = false;
  } else if (stateVal == 0b1110111) {
    state.mode = "off";
    state.off = true;
    state.lidOpen = true;
    state.airIn = false;
  }
  // Check for AUTO mode
  else if (stateVal == 0b1001011) {
    state.mode = "auto";
    state.off = false;
    state.lidOpen = false;
    state.airIn = false;
  } else if (stateVal == 0b1011011) {
    state.mode = "auto";
    state.off = false;
    state.lidOpen = false;
    state.airIn = true;
  }
  // Check for MANUAL mode
  else {
    state.mode = "manual";
    state.off = false;
    
    // Check lid state
    if (stateVal & 0b0100000) {  // STATE_OPEN bit
      state.lidOpen = true;
    } else {
      state.lidOpen = false;
    }
    
    // Check air direction
    if (stateVal & 0b0010000) {  // STATE_AIR_IN bit
      state.airIn = true;
    } else {
      state.airIn = false;
    }
  }
  
  // Decode speed
  state.speed = decodeSpeed(cmd.speed);
  if (state.speed < 0) {
    state.speed = 20; // Default speed
  }
  
  // Decode temperature
  state.temperature = decodeTemperature(cmd.temp);
  if (state.temperature < -2 || state.temperature > 37) {
    state.temperature = 20; // Default temperature
  }
  
  return state;
}

