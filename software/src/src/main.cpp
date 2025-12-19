#include <Arduino.h>
#include <MaxRemote.h>
#include <MaxReceiver.h>
#include <MaxFanBLE.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "Encoder.h"

// Instantiate the MaxRemote object on pin 2 (IR LED pin)
MaxRemote fanRemote(2);
MaxReceiver fanReceiver(2);  // TSOP4838 wired to GPIO2
MaxFanBLE fanBLE;

// OLED Display setup (0.96 inch SSD1306)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_SDA 6  // Seeed XIAO ESP32C3 default SDA
#define OLED_SCL 7  // Seeed XIAO ESP32C3 default SCL
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Rotary encoder pins (per user wiring)
const uint8_t ENCODER_PIN_A = 4;  // GPIO4
const uint8_t ENCODER_PIN_B = 5;  // GPIO5
Encoder encoder(ENCODER_PIN_A, ENCODER_PIN_B);

void drawEncoderValue(long value) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 16);
  display.println("Value:");
  display.setTextSize(3);
  display.setCursor(0, 40);
  display.println(value);
  display.display();
}

// Forward declarations (these are defined in MaxReceiver.cpp)
extern int binaryStringToInt(const String &binStr);
extern int decodeSpeed(const String &speedStr);
extern int decodeTemperature(const String &tempStr);

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
  
  // Decode temperature - need to reverse lookup the pattern
  int tempPattern = binaryStringToInt(cmd.temp);
  state.temperature = decodeTemperature(cmd.temp);
  
  // If decodeTemperature just returns the raw value, we need to do reverse lookup
  // For now, use a simple mapping - you may need to adjust this
  // The temperature pattern needs to be matched against the temperatureMappings array
  // This is a simplified version - you might need to implement proper reverse lookup
  if (state.temperature < -2 || state.temperature > 37) {
    state.temperature = 20; // Default temperature
  }
  
  return state;
}

// Callback function for BLE commands
void onBLECommand(const FanState& state) {
  Serial.println("BLE command received, sending IR signal...");
  
  // Convert FanState to MaxRemote parameters
  bool autoMode = (state.mode == "auto" || state.mode == "automatic");
  bool off = state.off;
  
  // Send IR command
  fanRemote.sendSignal(
    autoMode,           // autoMode
    state.temperature,  // temperature
    state.speed,        // fanSpeed
    state.lidOpen,      // lidOpen
    state.airIn,        // airIn
    off                 // off
  );
  
  Serial.println("IR signal sent");
}

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
  Serial.println("MaxxFan Controller started");
  
  // Initialize OLED display
  Wire.begin(OLED_SDA, OLED_SCL);
  Wire.setClock(400000); // Fast I2C to improve reliability

  // I2C scan to confirm display presence
  Serial.println("Scanning I2C bus...");
  uint8_t foundDevices = 0;
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.print("I2C device found at 0x");
      Serial.println(addr, HEX);
      foundDevices++;
    }
    delay(2);
  }
  if (foundDevices == 0) {
    Serial.println("No I2C devices found. Check wiring/power/SDA/SCL pins.");
  }
  Serial.println("I2C scan complete.");

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }
  
  // Initialize rotary encoder (handles interrupts internally)
  encoder.begin();
  encoder.reset();  // Start at zero position

  // Show initial encoder value
  drawEncoderValue(0);
  Serial.println("OLED display initialized with encoder value");

  
  // Initialize IR receiver
  fanReceiver.begin();
  
  // Initialize IR transmitter
  fanRemote.begin();
  
  // Initialize BLE
  fanBLE.begin("MaxxFan Controller");
  fanBLE.setCommandCallback(onBLECommand);
  
  Serial.println("Setup complete. Waiting for BLE connections or IR signals...");
}

void loop() {
  // Handle BLE connection/disconnection
  static bool wasConnected = false;
  bool isConnected = fanBLE.isConnected();
  
  if (isConnected && !wasConnected) {
    Serial.println("BLE client connected");
    wasConnected = true;
  } else if (!isConnected && wasConnected) {
    Serial.println("BLE client disconnected");
    wasConnected = false;
    // Restart advertising
    BLEDevice::startAdvertising();
  }
  
  // BLE commands are handled via callback in onBLECommand()
  
  // Check for IR signals received
  if (fanReceiver.available()) {
    Serial.println("\n=== IR Signal Received ===");
    
    // Get the raw bit string from the receiver.
    String rawBitString = fanReceiver.getBitString();
    Serial.print("Raw Bit String: ");
    Serial.println(rawBitString);
    
    // Parse the command
    MaxFanCommand cmd;
    if (fanReceiver.parseCommand(rawBitString, cmd)) {
      // Print the decoded command
      fanReceiver.printCommand(cmd);
      
      // Convert parsed command to FanState
      FanState receivedState = commandToFanState(cmd);
      
      Serial.println("\nConverted to FanState:");
      Serial.print("  Mode: "); Serial.println(receivedState.mode);
      Serial.print("  Temperature: "); Serial.println(receivedState.temperature);
      Serial.print("  Speed: "); Serial.println(receivedState.speed);
      Serial.print("  Lid Open: "); Serial.println(receivedState.lidOpen ? "true" : "false");
      Serial.print("  Air In: "); Serial.println(receivedState.airIn ? "true" : "false");
      Serial.print("  Off: "); Serial.println(receivedState.off ? "true" : "false");
      
      // Update BLE state
      fanBLE.setState(receivedState);
      Serial.println("BLE state updated and notified");
      
      // Re-emit the IR signal using raw data
      Serial.println("Re-emitting IR signal...");
      // Skip first value (gap) and use the rest
      if (fanReceiver.results.rawlen > 1) {
        // Copy volatile rawbuf to non-volatile array
        uint16_t rawData[fanReceiver.results.rawlen - 1];
        for (uint16_t i = 0; i < fanReceiver.results.rawlen - 1; i++) {
          rawData[i] = fanReceiver.results.rawbuf[i + 1];  // Skip first gap value
        }
        
        fanRemote.sendRaw(
          rawData,                        // Non-volatile copy
          fanReceiver.results.rawlen - 1, // Length minus gap
          38                              // 38kHz frequency
        );
        Serial.println("IR signal re-emitted");
      }
    } else {
      Serial.println("Failed to parse command on raw string.");
    }
    
    Serial.println("========================\n");
    
    // Prepare for the next IR signal.
    fanReceiver.resume();
  }

  // --- Rotary encoder handling via custom Encoder class (interrupt-based) ---
  static long lastDisplayedValue = 0;

  // Encoder is updated via interrupts internally - just read the position
  long pos = encoder.getPosition();

  if (pos != lastDisplayedValue) {
    drawEncoderValue(pos);
    lastDisplayedValue = pos;
    Serial.print("Encoder pos=");
    Serial.println(pos);
  } 
  
}