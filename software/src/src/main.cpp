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
#include <ButtonArray.h>
#include<MaxErrors.h>

#define ENCODER_BUTTON 8
#define MODE_BUTTON 10
#define COVER_BUTTON 9


MaxFanBLE fanBLE;
long testValue =0;

MaxFanState maxFanState;

MaxRemote fanRemote(2);
MaxReceiver fanIrReceiver(3);
ButtonArray buttons({ENCODER_BUTTON,MODE_BUTTON,COVER_BUTTON});
MaxFanDisplay fanDisplay(6, 7);
Encoder encoder(4, 5);


void onBLECommand(const String& json) {
  MaxError error=maxFanState.SetJson(json);
  if (error != MaxError::NONE)
    fanDisplay.showError(error);
}

void setup() {
  Serial.begin(115200);
  delay(2000); 

  // 1. Erst das Display initialisieren (hier wird Wire.begin() aufgerufen)
  if (!fanDisplay.begin()) { // Adresse meist 0x3C
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); 
  }

  // 2. ERST JETZT die Geschwindigkeit erhöhen
  Wire.setClock(400000); 
  
  buttons.begin();
  encoder.begin();
  encoder.reset();
  fanIrReceiver.begin();
  fanRemote.begin();
  fanBLE.setCommandCallback(onBLECommand);
  fanBLE.begin();
}

void loop() {
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

  fanRemote.send(maxFanState);

  fanDisplay.update(maxFanState, isConnected, testValue);
  
  if(fanBLE.isConnected()){
    fanBLE.notifyStatus(maxFanState.ToJson());
  }

  fanIrReceiver.update(maxFanState);

  int delta = encoder.getDelta();
  if(delta != 0){
    switch (maxFanState.GetMode())
    {
      case MaxFanMode::OFF:
        break;

      case MaxFanMode::MANUAL:
        maxFanState.SetSpeed(maxFanState.GetSpeed() - 10*delta);
        break;

      case MaxFanMode::AUTO:
        maxFanState.SetTempCelsius(maxFanState.GetTempCelsius()-delta);
        break;
        
      default:
        break;
    }
  }

  if(buttons.wasPressed(ENCODER_BUTTON)){
    Serial.println("ENCODER_BUTTON");
    maxFanState.SetAirFlow(maxFanState.GetAirFlow()==MaxFanDirection::IN ? MaxFanDirection::OUT: MaxFanDirection::IN);
    
  }
  
  if (buttons.wasPressed(COVER_BUTTON)){
    Serial.println("COVER_BUTTON");
    Serial.println(maxFanState.GetStateByte(),BIN);
    maxFanState.SetCover(maxFanState.GetCover()==CoverState::CLOSED ? CoverState::OPEN: CoverState::CLOSED);
    Serial.println(maxFanState.GetStateByte(),BIN);
    
  }
    

  if (buttons.wasPressed(MODE_BUTTON)){
    Serial.println("MODE_BUTTON");
    switch (maxFanState.GetMode())
    {
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
        if( maxFanState.GetMode() == MaxFanMode::OFF){
           Serial.println("OFF as commanded");
        } else {
          Serial.println("NOT OFF !!");
        }
        break;
        
      default:
        break;
    }
    
  }

  
}