#include "MaxReceiver.h"
using namespace MaxFan;

#include <stdlib.h>  // for strtol

// Use a separate tick constant for the receiver conversion.
#define RECEIVER_TICK_US 400

// --- Constructor ---
MaxReceiver::MaxReceiver(uint8_t recvPin) : irrecv(recvPin) {
  // Constructor: IRrecv is initialized with the given pin.
}

// --- begin() ---
void MaxReceiver::begin() {
  irrecv.enableIRIn();  // Start the IR receiver.
}

// --- available() ---
// Returns true if a new IR signal is available; fills 'results'.
bool MaxReceiver::available() {
  return irrecv.decode(&results);
}

// --- getBitString() ---
// Converts the raw durations (in microseconds) into a binary string.
// Uses RECEIVER_TICK_US as the conversion unit.
String MaxReceiver::getBitString() {
  String bitString = "";
  // Assume the received signal starts HIGH (you may try '0' if needed).
  char currentBit = '1';
  
  // Define a threshold to ignore very short durations (e.g., noise below 100 µs).
  const uint16_t threshold = 100;
  
  Serial.print("Processed raw durations: ");
  for (int i = 0; i < results.rawlen; i++) {
    uint16_t duration = results.rawbuf[i];
    if (duration < threshold) continue;
    
    Serial.print(duration);
    Serial.print(" ");
    
    // Determine the number of ticks (rounding to nearest integer).
    int count = (int)round((float)duration / RECEIVER_TICK_US);
    if (count < 1) count = 1;
    
    for (int j = 0; j < count; j++) {
      bitString += currentBit;
    }
    
    // Alternate the bit value.
    currentBit = (currentBit == '1') ? '0' : '1';
  }
  Serial.println();
  Serial.print("Final bit string length: ");
  Serial.println(bitString.length());
  return bitString;
}

// --- parseCommand() ---
// This version extracts only the essential fields: START, state (7 bits),
// separator, speed (7 bits), separator, and temp (7 bits). The tail is ignored.
bool MaxReceiver::parseCommand(const String &bitString, MaxFanCommand &cmd) {
  int lenStart = strlen(MaxFan::START);
  int lenSep = strlen(MaxFan::SEPARATOR);
  // Essential fields: START + state (7 bits) + separator + speed (7 bits) + separator + temp (7 bits)
  int expectedLength = lenStart + 7 + lenSep + 7 + lenSep + 7;
  
  Serial.print("Expected essential length: ");
  Serial.println(expectedLength);
  Serial.print("Received bit string length: ");
  Serial.println(bitString.length());
  
  // Locate the START sequence.
  int startIndex = bitString.indexOf(String(MaxFan::START));
  if (startIndex < 0) {
    Serial.println("START sequence not found.");
    return false;
  }
  
  // Trim the bit string from the START sequence.
  String trimmed = bitString.substring(startIndex);
  Serial.print("Trimmed bit string length: ");
  Serial.println(trimmed.length());
  
  // If the trimmed string is shorter than expected, pad with zeros.
  while (trimmed.length() < expectedLength) {
    trimmed += "0";
  }
  // If longer, cut to expected length.
  if (trimmed.length() > expectedLength) {
    trimmed = trimmed.substring(0, expectedLength);
  }
  Serial.print("Final bit string length for parsing: ");
  Serial.println(trimmed.length());
  
  int index = 0;
  cmd.start = trimmed.substring(index, index + lenStart);
  index += lenStart;
  
  cmd.state = trimmed.substring(index, index + 7);
  index += 7;
  
  cmd.separator1 = trimmed.substring(index, index + lenSep);
  index += lenSep;
  
  cmd.speed = trimmed.substring(index, index + 7);
  index += 7;
  
  cmd.separator2 = trimmed.substring(index, index + lenSep);
  index += lenSep;
  
  cmd.temp = trimmed.substring(index, index + 7);
  index += 7;
  
  // Debug: print parsed fields.
  Serial.println("Parsed essential fields:");
  Serial.print("Start: "); Serial.println(cmd.start);
  Serial.print("State (binary): "); Serial.println(cmd.state);
  Serial.print("Separator1: "); Serial.println(cmd.separator1);
  Serial.print("Speed (binary): "); Serial.println(cmd.speed);
  Serial.print("Separator2: "); Serial.println(cmd.separator2);
  Serial.print("Temp (binary): "); Serial.println(cmd.temp);
  
  // Validate that START and separators match.
  if (cmd.start != String(MaxFan::START)) {
    Serial.println("Start field does not match expected pattern.");
    return false;
  }
  if (cmd.separator1 != String(MaxFan::SEPARATOR) ||
      cmd.separator2 != String(MaxFan::SEPARATOR)) {
    Serial.println("Separator fields do not match expected pattern.");
    return false;
  }
  
  return true;
}

// --- Decoding helper functions ---
// Convert a binary string to an integer.
int binaryStringToInt(const String &binStr) {
  return strtol(binStr.c_str(), NULL, 2);
}

// Decodes the 7-bit state field into a human-readable description.
String decodeState(const String &stateStr) {
  int stateVal = binaryStringToInt(stateStr);
  // Example mappings – adjust based on your reverse-engineered values.
  if (stateVal == 0b0110111) return "Manual mode, Open, Air in";
  else if (stateVal == 0b0100111) return "Manual mode, Open, Air out";
  else if (stateVal == 0b0011111) return "Manual mode, Closed";
  else if (stateVal == 0b1011011) return "Automatic mode, Air in";
  else if (stateVal == 0b1001011) return "Automatic mode, Air out";
  else if (stateVal == 0b1111111) return "Off, Lid closed";
  else if (stateVal == 0b1110111) return "Off, Lid open";
  else return "Unknown state (" + String(stateVal) + ")";
}

// Decodes the 7-bit fan speed field into a percentage.
int decodeSpeed(const String &speedStr) {
  int speedVal = binaryStringToInt(speedStr);
  switch(speedVal) {
    case 0b1010111: return 10;
    case 0b1101011: return 20;
    case 0b1000011: return 30;
    case 0b1110101: return 40;
    case 0b1011001: return 50;
    case 0b1100001: return 60;
    case 0b1001110: return 70;
    case 0b1111010: return 80;
    case 0b1010010: return 90;
    case 0b1101100: return 100;
    default: return -1; // unknown
  }
}

// Decodes the 7-bit temperature field.
// (If your protocol requires bit reversal/inversion, implement that logic here.)
int decodeTemperature(const String &tempStr) {
  return binaryStringToInt(tempStr);
}

// --- printCommand() ---
// Prints the decoded command in human-readable form.
void MaxReceiver::printCommand(const MaxFanCommand &cmd) {
  Serial.println("Decoded MaxFan Command:");
  
  String stateDesc = decodeState(cmd.state);
  int speedPercent = decodeSpeed(cmd.speed);
  int temperature = decodeTemperature(cmd.temp);
  
  Serial.print("State: ");
  Serial.println(stateDesc);
  
  Serial.print("Fan Speed: ");
  if (speedPercent >= 0)
    Serial.print(String(speedPercent) + "%");
  else
    Serial.print("Unknown");
  Serial.println();
  
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" (raw value)");  // Adjust conversion if needed.
  
  // Optionally, also print raw binary fields for reference.
  Serial.println("Raw binary fields:");
  Serial.print("State: "); Serial.println(cmd.state);
  Serial.print("Speed: "); Serial.println(cmd.speed);
  Serial.print("Temp: "); Serial.println(cmd.temp);
}

// --- resume() ---
// Prepares the IR receiver for the next signal.
void MaxReceiver::resume() {
  irrecv.resume();
}
