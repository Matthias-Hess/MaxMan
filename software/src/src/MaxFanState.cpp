#include "MaxFanState.h"
#include "MaxRemote.h"  // For pattern constants and temperature mappings
#include <stdlib.h>  // for strtol

#define MAXFAN_BIT_ON         0  // Lüfter an/aus
#define MAXFAN_BIT_SPECIAL    1  // Auto-Modus oder Deckenlüfter-Modus
#define MAXFAN_BIT_OUT        2  // Richtung: 0=In 1=OUT
#define MAXFAN_BIT_COVER_OPEN 3  // Haube: 0=Zu, 1=Offen
#define MAXFAN_BIT_AUTO       4  // 0=Manual, 1=Auto


const uint8_t MASK_FAN_ON     = (1 << MAXFAN_BIT_ON);     // 0x01
const uint8_t MASK_SPECIAL    = (1 << MAXFAN_BIT_SPECIAL);   // 0x02
const uint8_t MASK_OUT        = (1 << MAXFAN_BIT_OUT); // 0x04
const uint8_t MASK_COVER_OPEN = (1 << MAXFAN_BIT_COVER_OPEN);    // 0x08
const uint8_t MASK_AUTO       = (1 << MAXFAN_BIT_AUTO);   

MaxFanState::MaxFanState() {
  stateByte = STATE_OFF;
  speedByte = FAN_SPEED_20;  
  tempFahrenheit = 78;  
}

uint8_t clampSpeed(uint8_t speed){
  speed =speed - (speed%10);
  if(speed>100)
    speed=100;
  if(speed<10)
    speed=10;
  return speed;
}

uint8_t clampToFahrenheit (uint8_t tempC){
  uint8_t f = (tempC * 9 / 5) + 32;
  if(f>99)
    f=99;
  if(f<29)
    f=29;
  return f;
}

// Set from raw bytes (for IR reception)
void MaxFanState::SetBytes(uint8_t state, uint8_t speed, uint8_t tempC) {
  stateByte = state & 0x7F;  // Mask to 7 bits
  speedByte = clampSpeed(speed);
  tempFahrenheit = clampToFahrenheit(tempC);
}

// Set from JSON string (for BLE reception)
bool MaxFanState::SetJson(const String& jsonString) {
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, jsonString);
  
  if (error) {
    return false;
  }

  if(doc.containsKey("mode")){

  }

  if (doc.containsKey("coverOpen")){

  }

  if (doc.containsKey("temperature")){

  }

  if (doc.containsKey("speed")){
    
  }

  if (doc.containsKey("airIn")){
    
  }
  

  
  
  
  return true;
}

// Convert to JSON string (for BLE transmission)
String MaxFanState::ToJson() const {
  StaticJsonDocument<200> doc;
  
  MaxFanMode mode = this->GetMode();
  switch (mode) {
    case MaxFanMode::OFF:    
      doc["mode"] = "OFF";
      break;

    case MaxFanMode::AUTO:    
      doc["mode"] = "AUTO";
      break;

    case MaxFanMode::MANUAL:    
      doc["mode"] = "MANUAL";
      break;
  }
  
  
  doc["coverOpen"] = GetCoverOpen();
  doc["temperature"] = GetTempCelsius();
  

  doc["speed"] = GetSpeed();
  
  // Include air flow direction
  
  
  
  String jsonString;
  serializeJson(doc, jsonString);
  return jsonString;
}


MaxFanMode MaxFanState::GetMode() const {
  if(((this->stateByte) & (MASK_FAN_ON)) ==0)
    return MaxFanMode::OFF;
  
  if(((this->stateByte) & MASK_AUTO) != 0)
      return MaxFanMode::AUTO;

  return MaxFanMode::MANUAL;
  
}

// Set state mode
void MaxFanState::SetMode(MaxFanMode mode) {
    switch (mode) {
        case MaxFanMode::OFF:
            this->stateByte &= ~(MASK_FAN_ON | MASK_SPECIAL);
            this->stateByte &= ~MASK_COVER_OPEN; 
            break;

        case MaxFanMode::AUTO:
            this->stateByte |= (MASK_FAN_ON | MASK_SPECIAL | MASK_AUTO);
            break;

        case MaxFanMode::MANUAL:
            this->stateByte |= MASK_FAN_ON;
            this->stateByte &= ~MASK_SPECIAL;
            this->stateByte |= MASK_COVER_OPEN;
            break;
    }
}

// Get temperature in Celsius
int MaxFanState::GetTempCelsius() const {
  // Convert Fahrenheit to Celsius: C = (F - 32) * 5/9
  return ((tempFahrenheit - 32) * 5) / 9;
}

// Set temperature in Celsius
bool MaxFanState::SetTempCelsius(int tempCelsius) {
  // Validate range: -2°C to 37°C (valid protocol range)
  if (tempCelsius < -2 || tempCelsius > 37) {
    return false;
  }
  // Convert Celsius to Fahrenheit: F = (C * 9/5) + 32
  tempFahrenheit = (int8_t)((tempCelsius * 9 / 5) + 32);
  return true;
}

// Get speed (1-10 scale)
int MaxFanState::GetSpeed() const {
  return this->speedByte;
}

// Set speed (1-10 scale)
void MaxFanState::SetSpeed(int speed) {
  this->speedByte = clampSpeed(speed);
}

// Get cover open state
bool MaxFanState::GetCoverOpen() const {
  return (this->stateByte & MASK_COVER_OPEN) != 0;
}

// Set cover open state
void MaxFanState::SetCoverOpen(bool open) {
  if(open)
    this->stateByte |= MASK_COVER_OPEN;
  else
    this->stateByte &= !MASK_COVER_OPEN;
}


MaxFanDirection MaxFanState::GetAirFlow() const {
  return ((this->stateByte & MASK_OUT)!=0) ? MaxFanDirection::OUT : MaxFanDirection::IN;
}

void MaxFanState::SetAirFlow(MaxFanDirection direction) {
  switch (direction) {
        case MaxFanDirection::OUT:
            this->stateByte |= MASK_OUT;
            break;

        case MaxFanDirection::IN:
            this->stateByte &= ~MASK_OUT;
            break;
    }
}




// Comparison operators
bool MaxFanState::operator==(const MaxFanState& other) const {
  return stateByte == other.stateByte &&
         speedByte == other.speedByte &&
         tempFahrenheit == other.tempFahrenheit;
}

bool MaxFanState::operator!=(const MaxFanState& other) const {
  return !(*this == other);
}










