#ifndef MAXREMOTE_H
#define MAXREMOTE_H

#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <MaxFanConstants.h>

#define TICK_US 800

// --- Predefined Bit Patterns (7 bits each) ---

// State patterns
static const uint8_t STATE_AUTO   = 0b1001011;  // Automatic mode
static const uint8_t STATE_MANUAL = 0b0000111;  // Manual mode
static const uint8_t STATE_OPEN   = 0b0100000;  // Lid open
static const uint8_t STATE_AIR_IN = 0b0010000;  // Air in
static const uint8_t STATE_CLOSED = 0b0001000;  // Lid closed
static const uint8_t STATE_OFF    = 0b1111111;  // Off (default lid closed)

// Fan speed patterns (10–100%)
static const uint8_t FAN_SPEED_10   = 0b1010111;
static const uint8_t FAN_SPEED_20   = 0b1101011;
static const uint8_t FAN_SPEED_30   = 0b1000011;
static const uint8_t FAN_SPEED_40   = 0b1110101;
static const uint8_t FAN_SPEED_50   = 0b1011001;
static const uint8_t FAN_SPEED_60   = 0b1100001;
static const uint8_t FAN_SPEED_70   = 0b1001110;
static const uint8_t FAN_SPEED_80   = 0b1111010;
static const uint8_t FAN_SPEED_90   = 0b1010010;
static const uint8_t FAN_SPEED_100  = 0b1101100;

// Temperature mapping structure
struct TempMapping {
  int temp;
  uint8_t pattern;
};

// Temperature mappings for -2°C to 37°C
static const TempMapping temperatureMappings[] = {
  { -2, 0b0100011 },
  { -1, 0b0000011 },
  {  0, 0b1111101 },
  {  1, 0b0111101 },
  {  2, 0b0011101 },
  {  3, 0b0101101 },
  {  4, 0b0001101 },
  {  5, 0b0110101 },
  {  6, 0b1010101 },
  {  7, 0b1100101 },
  {  8, 0b1000101 },
  {  9, 0b1111001 },
  { 10, 0b1011001 },
  { 11, 0b0011001 },
  { 12, 0b0101001 },
  { 13, 0b0001001 },
  { 14, 0b0110001 },
  { 15, 0b0010001 },
  { 16, 0b1100001 },
  { 17, 0b1000001 },
  { 18, 0b1111110 },
  { 19, 0b1011110 },
  { 20, 0b1101110 },
  { 21, 0b0101110 },
  { 22, 0b0001110 },
  { 23, 0b0110110 },
  { 24, 0b0010110 },
  { 25, 0b0100110 },
  { 26, 0b1000110 },
  { 27, 0b1111010 },
  { 28, 0b1011010 },
  { 29, 0b1101010 },
  { 30, 0b1001010 },
  { 31, 0b0001010 },
  { 32, 0b0110010 },
  { 33, 0b0010010 },
  { 34, 0b0100010 },
  { 35, 0b0000010 },
  { 36, 0b1111100 },
  { 37, 0b1011100 }
};

static const int numTempMappings = sizeof(temperatureMappings) / sizeof(temperatureMappings[0]);



// --- MaxRemote Class Definition ---
class MaxRemote {
  public:
    // Constructor: specify the IR LED output pin (e.g., pin 2)
    MaxRemote(uint8_t irPin);

    // Initialize the IR transmitter (call in setup())
    void begin();

    // Sends the IR signal based on the parameters.
    // autoMode: if true, use automatic mode (temperature is used)
    // temperature: desired temperature (only valid in auto mode)
    // fanSpeed: desired fan speed in percent (10,20,...,100, valid in manual mode)
    // lidOpen: true for open, false for closed (used in manual/off mode)
    // airIn: true for "air in" (false defaults to air out)
    // off: true to send the off signal (overrides most parameters)
    void sendSignal(bool autoMode, int temperature, int fanSpeed, bool lidOpen, bool airIn, bool off);

  private:
    IRsend irsend;
    String uint7ToBinaryString(uint8_t val);
    uint8_t getFanSpeedPattern(int speedPercent);
    bool getTemperaturePattern(int temp, uint8_t &pattern);
    void sendRawFromSignal(const String &binarySignal);
};

#endif // MAXREMOTE_H
