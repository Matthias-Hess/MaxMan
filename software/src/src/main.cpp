#include <Arduino.h>
#include <MaxRemote.h>
#include <MaxReceiver.h>
#include <MaxFanBLE.h>
#include <FanStateConverter.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "Encoder.h"


MaxRemote fanRemote(2);
MaxReceiver fanReceiver(3);  // TSOP4838 wired to GPIO3
MaxFanBLE fanBLE;

// Store the current command as canonical format (MaxFanCommand)
// This is the single source of truth for IR commands
MaxFanCommand currentCommand;
bool hasCurrentCommand = false;

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


// BLE command callback: convert FanState to MaxFanCommand and emit IR
void onBLECommand(const FanState& state) {
  Serial.println("BLE command received, converting and sending IR signal...");
  
  MaxFanCommand cmd = fanStateToCommand(state);
  currentCommand = cmd;
  hasCurrentCommand = true;
  
  fanRemote.sendCommand(cmd);
  Serial.println("IR signal sent");
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
  
  // Handle IR signals
  MaxFanCommand cmd;
  if (fanReceiver.getCommand(cmd)) {
    Serial.println("\n=== IR Signal Received ===");
    cmd.print();
    
    // Store as canonical format
    currentCommand = cmd;
    hasCurrentCommand = true;
    
    // Update BLE state
    FanState receivedState = commandToFanState(cmd);
    fanBLE.setState(receivedState);
    
    // Re-emit IR signal
    fanRemote.sendCommand(cmd);
    Serial.println("========================\n");
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