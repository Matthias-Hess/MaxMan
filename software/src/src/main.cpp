#include <Arduino.h>
#include <MaxRemote.h>
#include <MaxReceiver.h>
#include <MaxFanBLE.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "Encoder.h"
#include <MaxFanState.h>
#include <MaxFanDisplay.h>
#include <MaxErrors.h>

// HIER: Alte ButtonArray Lib raus, neue rein
#include "ChordInput.h" 

#define ENCODER_BUTTON 8
#define MODE_BUTTON 10
#define COVER_BUTTON 9

MaxFanBLE fanBLE;
long testValue = 0;

MaxFanState maxFanState;

MaxRemote fanRemote(2);
MaxReceiver fanIrReceiver(3);

ChordInput buttons({ENCODER_BUTTON, MODE_BUTTON, COVER_BUTTON});

MaxFanDisplay fanDisplay(6, 7);
Encoder encoder(4, 5);

void onBLECommand(const String& json) {
  MaxError error = maxFanState.SetJson(json);
  if (error != MaxError::NONE)
    fanDisplay.showError(error);
}

void setup() {
  Serial.begin(115200);
  delay(2000); 

  // 1. Erst das Display initialisieren
  if (!fanDisplay.begin()) { 
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); 
  }

  // 2. ERST JETZT die Geschwindigkeit erhöhen
  Wire.setClock(400000); 
  
  // HIER: buttons.begin() entfällt, da Setup jetzt im Konstruktor passiert!
  
  encoder.begin();
  encoder.reset();
  fanIrReceiver.begin();
  fanRemote.begin();
  fanBLE.setCommandCallback(onBLECommand);
  fanBLE.begin();
}

void loop() {
  static unsigned long lastButtonCheck = 0;
  if (millis() - lastButtonCheck >= 20) {
      buttons.tick(); 
      lastButtonCheck = millis();
  }

  static bool wasConnected = false;
  bool isConnected = fanBLE.isConnected();
  
  if (isConnected && !wasConnected) {
    Serial.println("BLE client connected");
    wasConnected = true;
  } else if (!isConnected && wasConnected) {
    Serial.println("BLE client disconnected");
    wasConnected = false;
    BLEDevice::startAdvertising();
  }

  fanRemote.send(maxFanState);
  fanDisplay.update(maxFanState, isConnected, testValue);
  
  if(fanBLE.isConnected()){
    fanBLE.notifyStatus(maxFanState.ToJson());
  }

  fanIrReceiver.update(maxFanState);

  int delta = encoder.getDelta();
  if(delta != 0){
    switch (maxFanState.GetMode()) {
      case MaxFanMode::OFF: break;
      case MaxFanMode::MANUAL:
        maxFanState.SetSpeed(maxFanState.GetSpeed() - 10*delta);
        break;
      case MaxFanMode::AUTO:
        maxFanState.SetTempCelsius(maxFanState.GetTempCelsius()-delta);
        break;
      default: break;
    }
  }

    if (buttons.hasEvent()) {
      
      // Das Event aus der Queue holen
      KeyEvent event = buttons.popEvent();

      // --- Fall A: Encoder Button gedrückt ---
      if (event.IsSingle(ENCODER_BUTTON)) {
          Serial.println("EVENT: ENCODER_BUTTON");
          maxFanState.SetAirFlow(maxFanState.GetAirFlow()==MaxFanDirection::IN ? MaxFanDirection::OUT: MaxFanDirection::IN);
      }
      
      // --- Fall B: Cover Button gedrückt ---
      else if (event.IsSingle(COVER_BUTTON)) {
          Serial.println("EVENT: COVER_BUTTON");
          // Debugging Output beibehalten
          Serial.println(maxFanState.GetStateByte(), BIN);
          maxFanState.SetCover(maxFanState.GetCover()==CoverState::CLOSED ? CoverState::OPEN: CoverState::CLOSED);
          Serial.println(maxFanState.GetStateByte(), BIN);
      }

      // --- Fall C: Mode Button gedrückt ---
      else if (event.IsSingle(MODE_BUTTON)) {
          Serial.println("EVENT: MODE_BUTTON");
          switch (maxFanState.GetMode()) {
            case MaxFanMode::OFF:
              Serial.println("OFF->MANUAL");
              maxFanState.SetMode(MaxFanMode::MANUAL);
              break;
            case MaxFanMode::MANUAL:
              Serial.println("MANUAL->AUTO");
              maxFanState.SetMode(MaxFanMode::AUTO);
              break;
            case MaxFanMode::AUTO:
              Serial.println("AUTO->OFF");
              maxFanState.SetMode(MaxFanMode::OFF);
              // Debug Check wie im Original
              if(maxFanState.GetMode() == MaxFanMode::OFF){
                 Serial.println("OFF as commanded");
              } else {
                 Serial.println("NOT OFF !!");
              }
              break;
            default: break;
          }
      }

      // --- Fall D: NEU! Kombinationen (Beispiel) ---
      // Hier könntest du jetzt magische Kombinationen einbauen
      else if (event.IsChord(MODE_BUTTON, COVER_BUTTON)) {
          Serial.println("EVENT: MODE + COVER gleichzeitig!");
          Serial.println("Secret Reset triggered...");
          // maxFanState.Reset(); // oder ähnliches
      }
  }
}