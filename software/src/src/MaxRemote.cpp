#include "MaxRemote.h"
#include "MaxReceiver.h"
#include <stdlib.h>  // for strtol
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
  
  
}

// Send raw IR data directly (for re-emitting received signals)
void MaxRemote::sendRaw(const uint16_t* rawData, uint16_t length, uint16_t frequency) {
  irsend.sendRaw(rawData, length, frequency);
}



// Helper: Convert binary string to uint8_t
static uint8_t binaryStringToUint8(const String &binStr) {
  return (uint8_t)strtol(binStr.c_str(), NULL, 2);
}

// Send IR command from MaxFanCommand structure (canonical format)
void MaxRemote::sendCommand(const MaxFanState& cmd) {
  
  
}