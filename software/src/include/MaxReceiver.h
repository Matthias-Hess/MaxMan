#ifndef MAXRECEIVER_H
#define MAXRECEIVER_H
#include <MaxFanConstants.h>
#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>  // Useful for utility functions (optional)
#include <math.h>

// Define the base time unit (microseconds) used by the sender.
#define TICK_US 800




// Field lengths (in bits)
static const int STATE_LEN    = 7;
static const int SPEED_LEN    = 7;
static const int TEMP_LEN     = 7;
static const int CHECKSUM_LEN = 7;
static const int UNKNOWN_LEN  = 18;

// Structure to hold a parsed MaxxFan command.
struct MaxFanCommand {
  String start;
  String state;
  String separator1;
  String speed;
  String separator2;
  String temp;
  String separator3;
  String unknown;
  String separator4;
  String checksum;
  String end;
};

class MaxReceiver {
  public:
    // Constructor: specify the IR receiver pin (e.g. the pin where your TSOP4838 is connected)
    MaxReceiver(uint8_t recvPin);

    // Initialize the IR receiver (call this in setup())
    void begin();

    // Check if a new IR signal has been received.
    // This function calls irrecv.decode() and returns true if a signal is available.
    bool available();

    // Convert the raw received IR signal into a binary string.
    // This uses the TICK_US constant to approximate how many "ticks" each duration represents.
    String getBitString();

    // Parse a given bit string into its constituent command fields.
    // Returns true if parsing is successful (i.e. the expected separators, start, and end are found).
    bool parseCommand(const String &bitString, MaxFanCommand &cmd);

    // Print the parsed command fields to Serial.
    void printCommand(const MaxFanCommand &cmd);

    // Resume the IR receiver to be ready for the next signal.
    void resume();
    decode_results results;
  private:
    IRrecv irrecv;
    
};

#endif // MAXRECEIVER_H
