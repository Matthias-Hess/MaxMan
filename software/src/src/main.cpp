#include <Arduino.h>
#include <MaxRemote.h>
#include <MaxReceiver.h>



#include "MaxRemote.h"

// Instantiate the MaxRemote object on pin 2 (IR LED pin)
MaxRemote fanRemote(2);
MaxReceiver fanReceiver(3);  // Use the appropriate digital pin for your TSOP4838

// Helper function: invert a bit string (swap '1' and '0')
String invertBitString(const String &input) {
  String inverted = "";
  for (int i = 0; i < input.length(); i++) {
    char c = input.charAt(i);
    if (c == '1')
      inverted += '0';
    else if (c == '0')
      inverted += '1';
    else
      inverted += c;
  }
  return inverted;
}





void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("MaxReceiver started");
  fanReceiver.begin();
}

void loop() {
  if (fanReceiver.available()) {
    // Get the raw bit string from the receiver.
    String rawBitString = fanReceiver.getBitString();
    Serial.print("Raw Bit String: ");
    Serial.println(rawBitString);
    
    // Use the raw bit string (without inversion) since it starts with the expected START.
    MaxFanCommand cmd;
    if (fanReceiver.parseCommand(rawBitString, cmd)) {
      fanReceiver.printCommand(cmd);
    } else {
      Serial.println("Failed to parse command on raw string.");
    }
    
    // Prepare for the next IR signal.
    fanReceiver.resume();
  }
}