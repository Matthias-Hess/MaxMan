#ifndef MAXREMOTE_H
#define MAXREMOTE_H

#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <MaxFanConstants.h>
#include <MaxFanState.h>

#define TICK_US 800


class MaxRemote {
  public:
    MaxRemote(uint8_t irPin);
    void begin();
    void send(MaxFanState& state);
  private:
    IRsend irsend;
    MaxFanState lastSentState;
    unsigned long lastSentAtUptimeMilliseconds =0;
};

#endif
