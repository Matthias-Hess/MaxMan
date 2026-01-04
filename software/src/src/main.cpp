#include <Arduino.h>
#include <Wire.h>
#include <Preferences.h>
#include <WiFi.h>
// --- Deine Bibliotheken ---
#include <MaxRemote.h>
#include <MaxReceiver.h>
#include <MaxFanBLE.h>
#include <MaxFanMQTT.h>
#include <MaxFanState.h>
#include <MaxFanDisplay.h> // Deine alte Display Klasse (für Standard Mode)
#include <MaxErrors.h>
#include "MaxFanConfig.h"
#include "RemoteAccess.h"

// --- Input & Grafik ---
#include "Encoder.h"
#include "ChordInput.h" 
#include <U8g2lib.h>       // Für das Menü (wie gewünscht)

// --- Die neuen App-Modes ---
#include "AppMode.h"
#include "ModeStandard.h"
#include "ModeConfig.h"
#include "ModeScreenDark.h"

// --- Pin Definitionen ---
#define ENCODER_BUTTON 8
#define MODE_BUTTON 10
#define COVER_BUTTON 9

// --- Globale Hardware Instanzen ---

// 1. Core Logic
MaxFanState maxFanState;
MaxFanBLE fanBLE;
MaxFanMQTT fanMQTT;
MaxRemote fanRemote(2);
MaxReceiver fanIrReceiver(3);


// 2. Inputs
ChordInput buttons({ENCODER_BUTTON, MODE_BUTTON, COVER_BUTTON});
Encoder encoder(4, 5);

// 3. Displays
// ACHTUNG: Wir haben hier zwei Objekte für dasselbe Display.
// ModeStandard nutzt 'fanDisplay' (Adafruit Wrapper), ModeConfig nutzt 'u8g2'.
// Das funktioniert, solange man beim Umschalten sauber cleared.
MaxFanDisplay fanDisplay(6, 7); 

// Beispiel-Konstruktor für SSD1306 via I2C (bitte an dein Display anpassen!)
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);


// --- State Machine Variablen ---
AppMode* currentMode = nullptr;
ModeStandard* modeStandard = nullptr;
ModeConfig* modeConfig = nullptr;
ModeScreenDark* modeScreenDark = nullptr;
RemoteAccess* activeRemote = nullptr;


// --- Callbacks ---

// BLE Callback muss global oder statisch bleiben
void onBLECommand(const String& json) {
  MaxError error = maxFanState.SetJson(json);
  if (error != MaxError::NONE)
    fanDisplay.showError(error);
}

// Hilfsfunktion zum Umschalten
void switchMode(AppMode* newMode) {
  if (currentMode != newMode) {
    currentMode = newMode;
    currentMode->enter(); // Setup für den neuen Screen aufrufen
  }
}

void setup() {
  Serial.begin(115200);
  delay(2000); 

  ConfigManager::load();

  // If MQTT is selected, try to connect to WiFi using stored credentials
  if (GlobalConfig.connection == 2) {
    Serial.print("Attempting WiFi connect to: ");
    Serial.println(GlobalConfig.wifiSSID);
    if (strlen(GlobalConfig.wifiSSID) > 0) {
      WiFi.begin(GlobalConfig.wifiSSID, GlobalConfig.wifiPassword);
      unsigned long start = millis();
      while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
        delay(500);
        Serial.print('.');
      }
      Serial.println();
      if (WiFi.status() == WL_CONNECTED) {
        Serial.print("WiFi connected, IP: ");
        Serial.println(WiFi.localIP());
      } else {
        Serial.println("WiFi connect failed");
      }
    }
  }

  Serial.print("Booting Version: ");
  Serial.println(APP_VERSION);

  // 1. Hardware Initialisierung
  // Wire Clock erst setzen, nachdem Displays ggf. initiiert wurden
  // (Je nachdem wie fanDisplay.begin implementiert ist)
  if (!fanDisplay.begin()) { 
    Serial.println(F("SSD1306 allocation failed (Standard Lib)"));
    for (;;); 
  }
  
  u8g2.begin(); // U8g2 initialisieren
  
  Wire.setClock(400000); 

  encoder.begin();
  encoder.reset();
  
  fanIrReceiver.begin();
  fanRemote.begin();
  
    fanBLE.setCommandCallback(onBLECommand);
    fanBLE.begin();

    fanMQTT.setCommandCallback(onBLECommand);
    fanMQTT.begin();

    // Select active remote based on config
    if (GlobalConfig.connection == 2) {
      activeRemote = &fanMQTT;
      Serial.println("Using MQTT Remote Access");
    } else if (GlobalConfig.connection == 1) {
      activeRemote = &fanBLE;
      Serial.println("Using BLE Access");
    } else {
      activeRemote = nullptr;
    }

  // 2. Modi Instanziieren (Dependency Injection)
  // Wir übergeben alle Hardware-Objekte, die der jeweilige Mode braucht.
  
    modeStandard = new ModeStandard(
      u8g2,          
      encoder, 
      buttons,
      maxFanState, 
      fanDisplay,     
      fanRemote, 
      fanIrReceiver, 
      (activeRemote ? *activeRemote : fanBLE)
    );

  modeConfig = new ModeConfig(
      &u8g2,           
      &encoder, 
      &buttons
      
  );

    modeScreenDark = new ModeScreenDark(
      u8g2,
      encoder,
      buttons,
      maxFanState,
      fanRemote,
      fanIrReceiver,
      (activeRemote ? *activeRemote : fanBLE)
    );

  // 3. Start-Modus setzen
  switchMode(modeStandard);
}

void loop() {
  
  // --- A. Globale Input Pflege ---
  // Das muss hier passieren, damit das Entprellen (Debounce) 
  // unabhängig von der Ausführungszeit des aktuellen Modes funktioniert.
  static unsigned long lastButtonCheck = 0;
  if (millis() - lastButtonCheck >= 20) {
      buttons.tick(); 
      lastButtonCheck = millis();
  }

  // --- B. Aktuellen Modus ausführen ---
  if (currentMode) {
      // 1. Loop des Modes aufrufen
      ModeAction action = currentMode->loop();

      // 2. Prüfen, ob der Modus wechseln will
      switch (action) {
          case ModeAction::SWITCH_TO_CONFIG:
              Serial.println(F("Main: Switching to Config"));
              switchMode(modeConfig);
              break;
              
          case ModeAction::SWITCH_TO_STANDARD:
              Serial.println(F("Main: Switching to Standard"));
              switchMode(modeStandard);
              break;
              
          case ModeAction::SWITCH_TO_SCREEN_DARK:
              Serial.println(F("Main: Switching to Screen Dark"));
              switchMode(modeScreenDark);
              break;
              
          case ModeAction::NONE:
          default:
              break;
      }
  }

  // Ensure the active remote can service its background tasks (MQTT loop, reconnection)
  if (activeRemote) {
      activeRemote->loop();
  }
}