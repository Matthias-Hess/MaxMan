#ifndef MAXRECEIVER_H
#define MAXRECEIVER_H
#include <MaxFanConstants.h>
#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>  // Useful for utility functions (optional)
#include <math.h>
#include <MaxFanState.h>

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



class MaxReceiver {
  public:
    MaxReceiver(uint8_t recvPin);
    bool update(MaxFanState & cmd);
    void begin();

    
    
    
  private:
    IRrecv irrecv;
    decode_results results;
    bool parseToBytes(uint8_t* output16Bytes);
    void resume();
};

#endif // MAXRECEIVER_H
