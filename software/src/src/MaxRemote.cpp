#include "MaxRemote.h"
using namespace MaxFan;
// Constructor: Initialize the IRsend instance with the given pin.
MaxRemote::MaxRemote(uint8_t irPin) : irsend(irPin) { }

// Initialize the IR transmitter.
void MaxRemote::begin() {
  // IRremoteESP8266 requires a call to begin() on IRsend.
  irsend.begin();
}

// Helper: Convert a 7-bit value into a binary string (MSB first)
String MaxRemote::uint7ToBinaryString(uint8_t val) {
  String s = "";
  for (int i = 6; i >= 0; i--) {
    s += ((val >> i) & 1) ? "1" : "0";
  }
  return s;
}

// Helper: Map a fan speed percentage to its corresponding 7-bit pattern.
uint8_t MaxRemote::getFanSpeedPattern(int speedPercent) {
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
    default:   return FAN_SPEED_20; // Default if an invalid value is provided.
  }
}

// Helper: Look up the 7-bit pattern for a given temperature.
// Returns true if found, false otherwise.
bool MaxRemote::getTemperaturePattern(int temp, uint8_t &pattern) {
  for (int i = 0; i < numTempMappings; i++) {
    if (temperatureMappings[i].temp == temp) {
      pattern = temperatureMappings[i].pattern;
      return true;
    }
  }
  return false;
}

// Helper: Convert the binary string into an array of pulse durations and send it.
// The durations are calculated using TICK_US as the base unit.
void MaxRemote::sendRawFromSignal(const String &binarySignal) {
  int len = binarySignal.length();
  // Worst-case: each bit results in one duration.
  uint16_t durations[len];
  int count = 1;
  char current = binarySignal.charAt(0);
  int durationCount = 0;

  for (int i = 1; i < len; i++) {
    char bit = binarySignal.charAt(i);
    if (bit == current) {
      count++;
    } else {
      durations[durationCount++] = count * TICK_US;
      current = bit;
      count = 1;
    }
  }
  // Handle the final group.
  durations[durationCount++] = count * TICK_US;

  // Send the raw IR signal at 38 kHz.
  irsend.sendRaw(durations, durationCount, 38);
}

// Build and send the complete IR signal based on the parameters.
void MaxRemote::sendSignal(bool autoMode, int temperature, int fanSpeed, bool lidOpen, bool airIn, bool off) {
  uint8_t state = 0;
  uint8_t speed = 0;
  uint8_t tempVal = 0;

  const int defaultTemp = 20;
  const int defaultFanSpeed = 20;

  if (autoMode) {
    // Automatic mode: use the auto state; add air_in if specified.
    state = STATE_AUTO;
    if (airIn) {
      state |= STATE_AIR_IN;
    }
    speed = getFanSpeedPattern(defaultFanSpeed);
    if (!getTemperaturePattern(temperature, tempVal)) {
      // Fallback to default temperature.
      getTemperaturePattern(defaultTemp, tempVal);
    }
  } else if (off) {
    // Off mode: use the off state. Modify lid status if requested.
    state = STATE_OFF;
    if (lidOpen) {
      // Clear bit 3 (counting from the left) to indicate an open lid.
      state &= ~(1 << 3);
    }
    speed = getFanSpeedPattern(defaultFanSpeed);
    getTemperaturePattern(defaultTemp, tempVal);
  } else {
    // Manual mode: combine manual state with lid and airflow settings.
    state = STATE_MANUAL;
    if (lidOpen) {
      state |= STATE_OPEN;
    } else {
      state |= STATE_CLOSED;
    }
    if (airIn) {
      state |= STATE_AIR_IN;
    }
    speed = getFanSpeedPattern(fanSpeed);
    // Temperature defaults to 20°C in manual mode.
    getTemperaturePattern(defaultTemp, tempVal);
  }

  // Calculate checksum:
  // For each bit (from leftmost, index 0):
  // - For bits 0,1,5: checksum[i] = state[i] XOR speed[i] XOR temp[i]
  // - For bits 2,3,4,6: checksum[i] = NOT (state[i] XOR speed[i] XOR temp[i])
  uint8_t checksum = 0;
  for (int i = 0; i < 7; i++) {
    uint8_t sbit = (state >> (6 - i)) & 1;
    uint8_t fbit = (speed >> (6 - i)) & 1;
    uint8_t tbit = (tempVal >> (6 - i)) & 1;
    uint8_t xorVal = sbit ^ fbit ^ tbit;
    if (i == 2 || i == 3 || i == 4 || i == 6) {
      xorVal = (xorVal == 0) ? 1 : 0;
    }
    checksum |= (xorVal << (6 - i));
  }

  // Convert the 7-bit values to binary strings.
  String stateStr    = uint7ToBinaryString(state);
  String speedStr    = uint7ToBinaryString(speed);
  String tempStr     = uint7ToBinaryString(tempVal);
  String checksumStr = uint7ToBinaryString(checksum);

  // Construct the complete binary signal:
  // START + state + SEPARATOR + speed + SEPARATOR + temp +
  // SEPARATOR + UNKNOWN_FIELD + SEPARATOR + checksum + END
  String finalSignal = String(START) + stateStr + String(SEPARATOR) +
                       speedStr + String(SEPARATOR) + tempStr + String(SEPARATOR) +
                       String(UNKNOWN_FIELD) + String(SEPARATOR) + checksumStr + String(END);

  // Send the constructed signal.
  sendRawFromSignal(finalSignal);
}
