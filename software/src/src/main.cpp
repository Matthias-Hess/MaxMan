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

#define ENCODER_BUTTON 8
#define MODE_BUTTON 10
#define AIRFLOW_BUTTON 9


MaxFanBLE fanBLE;
long testValue =0;

MaxFanState maxFanState;

MaxRemote fanRemote(2);
MaxReceiver fanIrReceiver(3);
ButtonArray buttons({ENCODER_BUTTON,MODE_BUTTON,AIRFLOW_BUTTON});
MaxFanDisplay fanDisplay(6, 7);
Encoder encoder(4, 5);


void onBLECommand(const String& json) {
  maxFanState.SetJson(json);
}

void setup() {
  Serial.begin(115200);
  delay(2000); 
  Serial.println("Hello");

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

  fanDisplay.update(maxFanState, isConnected, testValue);
  fanRemote.send(maxFanState);
  fanIrReceiver.update(maxFanState);

  int delta = encoder.getDelta();
  if(delta != 0){
    testValue+=delta;
    Serial.println(testValue);
  }

  if(buttons.wasPressed(ENCODER_BUTTON)){
    Serial.println("ENCODER_BUTTON");
    testValue+=1;
  }
  
  if (buttons.wasPressed(AIRFLOW_BUTTON)){
    Serial.println("AIRFLOW_BUTTON");
    testValue +=1;
  }
    

  if (buttons.wasPressed(MODE_BUTTON)){
    Serial.println("MODE_BUTTON");
    testValue +=1;
  }

  
}