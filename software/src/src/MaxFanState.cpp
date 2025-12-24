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

std::string toString(MaxFanMode mode) {
    switch(mode) {
        case MaxFanMode::OFF: return "OFF";
        case MaxFanMode::AUTO: return "AUTO";
        case MaxFanMode::MANUAL: return "MANUAL";
        default: throw std::invalid_argument("Unknown MaxFanMode");
    }
}

MaxFanMode toMaxFanMode(const std::string& str) {
    if(str == "OFF") return MaxFanMode::OFF;
    if(str == "AUTO") return MaxFanMode::AUTO;
    if(str == "MANUAL") return MaxFanMode::MANUAL;
    throw std::invalid_argument("Invalid MaxFanMode string: " + str);
}

std::string toString(MaxFanDirection dir) {
    switch(dir) {
        case MaxFanDirection::IN: return "IN";
        case MaxFanDirection::OUT: return "OUT";
        default: throw std::invalid_argument("Unknown MaxFanMode");
    }
}

MaxFanDirection toMaxFanDirection(const std::string& str) {
    if(str == "IN") return MaxFanDirection::IN;
    if(str == "OUT") return MaxFanDirection::OUT;
    
    throw std::invalid_argument("Invalid toMaxFanDirection string: " + str);
}

std::string toString(CoverState mode) {
    switch(mode) {
        case CoverState::OPEN: return "OPEN";
        case CoverState::CLOSED: return "CLOSED";
        default: throw std::invalid_argument("Unknown CoverState");
    }
}

CoverState toCoverState(const std::string& str) {
    if(str == "OPEN") return CoverState::OPEN;
    if(str == "AUTO") return CoverState::CLOSED;
    throw std::invalid_argument("Invalid CoverState string: " + str);
}

MaxFanState::MaxFanState() {
  this->SetMode(MaxFanMode::OFF);
  this->SetSpeed(20);
  this->SetTempCelsius(26);
}

uint8_t clampSpeed(uint8_t speed){
  speed =speed - (speed%10);
  if(speed>100)
    speed=100;
  if(speed<10)
    speed=10;
  return speed;
}
uint8_t clampTF(uint8_t tempF){
  if(tempF>99)
    return 99;
  if(tempF<29)
    return 29;
  return tempF;
}
uint8_t clampToFahrenheit (uint8_t tempC){
  
    float fahrenheit = (tempC * 1.8f) + 32.0f;
    uint8_t f;
    if(tempC >=0)
      f = (uint8_t)floor(fahrenheit);
    else
      f = (uint8_t)ceil(fahrenheit);

    return clampTF(f);
}



// Set from raw bytes (for IR reception)
void MaxFanState::SetBytes(uint8_t state, uint8_t speed, uint8_t tempF) {
  stateByte = state;  
  speedByte = clampSpeed(speed);
  tempFahrenheit = clampTF(tempF);
}

// Set from JSON string (for BLE reception)
bool MaxFanState::SetJson(const String& jsonString) {
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, jsonString);
  
  if (error) {
    return false;
  }

  if(doc.containsKey("mode")){
    SetMode(toMaxFanMode(doc["mode"]));
  }

  if (doc.containsKey("cover")){
    SetCover(toCoverState(doc["cover"]));
  }

  if (doc.containsKey("airflow")){
    SetAirFlow(toMaxFanDirection(doc["airflow"]));
  }

  if (doc.containsKey("speed")){
    uint16_t speed = doc["speed"].as<uint16_t>();
    SetSpeed(speed);
    
  }

  if (doc.containsKey("temperature")){
    uint16_t tempC = doc["temperature"].as<uint16_t>();
    SetTempCelsius(tempC);
  }

  return true;
}

// Convert to JSON string (for BLE transmission)
String MaxFanState::ToJson() const {
  StaticJsonDocument<200> doc;
  doc["mode"] = toString(GetMode());
  doc["cover"] = toString(GetCover());
  doc["airflow"] = toString(GetAirFlow());
  doc["speed"] = GetSpeed();
  doc["temperature"] = GetTempCelsius();
  String jsonString;
  serializeJson(doc, jsonString);
  return jsonString;
}


MaxFanMode MaxFanState::GetMode() const {
  
  
  if(((this->stateByte) & MASK_AUTO) != 0)
      return MaxFanMode::AUTO;
  
  if(((this->stateByte) & (MASK_FAN_ON)) ==0)
    return MaxFanMode::OFF;

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
  return (int) round((tempFahrenheit-32.0)/1.8);
}

// Set temperature in Celsius
bool MaxFanState::SetTempCelsius(int tempCelsius) {
  tempFahrenheit = clampToFahrenheit(tempCelsius);
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
CoverState MaxFanState::GetCover() const {
  return (this->stateByte & MASK_COVER_OPEN) != 0 
  ? CoverState::OPEN
  : CoverState::CLOSED;
}

// Set cover open state
void MaxFanState::SetCover(CoverState state) {
  if(state==CoverState::OPEN)
    this->stateByte |= MASK_COVER_OPEN;
  else
    this->stateByte &= !MASK_COVER_OPEN;
}


MaxFanDirection MaxFanState::GetAirFlow() const {
  return ((this->stateByte & MASK_OUT)!=0) 
    ? MaxFanDirection::OUT 
    : MaxFanDirection::IN;
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










