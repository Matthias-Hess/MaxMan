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

// Forward declarations for decode helper functions (defined in MaxReceiver.cpp)
String decodeState(const String &stateStr);
int decodeSpeed(const String &speedStr);
int decodeTemperature(const String &tempStr);

// Structure to hold a parsed MaxxFan command.
// Only stores the actual data fields - constants (start, separators, unknown, end, checksum)
// are reconstructed from MaxFanConstants when emitting.
struct MaxFanCommand {
  String state;   // 7-bit binary string
  String speed;   // 7-bit binary string
  String temp;    // 7-bit binary string
  
  // Print the command in human-readable form (for debugging)
  void print() const;
};

class MaxReceiver {
  public:
    // Constructor: specify the IR receiver pin (e.g. the pin where your TSOP4838 is connected)
    MaxReceiver(uint8_t recvPin);

    // Initialize the IR receiver (call this in setup())
    void begin();

    // Get the latest received command, if available
    // This handles decoding, parsing, and resume internally
    // 
    // Returns true if a new command was received and parsed successfully, false otherwise.
    // If true, the command is copied into the provided reference parameter.
    // Returns false in the vast majority of cases (no signal received).
    bool getCommand(MaxFanCommand& cmd);
    
  private:
    IRrecv irrecv;
    decode_results results;
    
    // Internal storage for the last parsed command
    MaxFanCommand lastCommand;
    bool hasLastCommand;
    
    // Internal helper methods
    String getBitString();
    bool parseCommand(const String &bitString, MaxFanCommand &cmd);
    void resume();
    
};

#endif // MAXRECEIVER_H
