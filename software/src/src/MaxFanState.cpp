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
        default: return "???";
    }
}

// 1. Robuster Integer Parser
// Akzeptiert int und float (z.B. 50.0), lehnt aber Strings/Bools ab.
// Prüft min/max Grenzen.
bool tryParseInt(JsonVariant variant, int& outVal, int min, int max) {
    // Check: Ist es überhaupt eine Zahl? (Verhindert "true" oder "text")
    if (!variant.is<float>() && !variant.is<int>()) {
        return false;
    }

    // Wert holen (als float, um 50.0 zu unterstützen)
    float fVal = variant.as<float>();
    int val = (int)fVal;

    // Bereichsprüfung
    if (val < min || val > max) {
        return false;
    }

    outVal = val;
    return true;
}

// 2. Enum Parser (Zero-Copy mit const char*)
bool tryParseFanMode(const char* str, MaxFanMode& outMode) {
    if (!str) return false;
    if (strcasecmp(str, "OFF") == 0)    { outMode = MaxFanMode::OFF; return true; }
    if (strcasecmp(str, "AUTO") == 0)   { outMode = MaxFanMode::AUTO; return true; }
    if (strcasecmp(str, "MANUAL") == 0) { outMode = MaxFanMode::MANUAL; return true; }
    return false;
}

std::string toString(MaxFanDirection dir) {
    switch(dir) {
        case MaxFanDirection::IN: return "IN";
        case MaxFanDirection::OUT: return "OUT";
        default: return "???";
    }
}

bool tryParseFanDirection(const char* str, MaxFanDirection& outDir) {
    if (!str) return false;
    if (strcasecmp(str, "IN") == 0)  { outDir = MaxFanDirection::IN; return true; }
    if (strcasecmp(str, "OUT") == 0) { outDir = MaxFanDirection::OUT; return true; }
    return false;
}

MaxFanDirection toMaxFanDirection(const std::string& str) {
  MaxFanDirection d = MaxFanDirection::IN;
  if (tryParseFanDirection(str.c_str(), d)) return d;
  return MaxFanDirection::IN;
}

std::string toString(CoverState mode) {
    switch(mode) {
        case CoverState::OPEN: return "OPEN";
        case CoverState::CLOSED: return "CLOSED";
        default: return "???";
    }
}


bool tryParseCoverState(const char* str, CoverState& outCover) {
    if (!str) return false;
    if (strcasecmp(str, "OPEN") == 0)   { outCover = CoverState::OPEN; return true; }
    if (strcasecmp(str, "CLOSED") == 0) { outCover = CoverState::CLOSED; return true; }
    return false;
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
MaxError MaxFanState::SetJson(const String& jsonString) {
    // Reserviere Puffer (StaticJsonDocument auf Stack -> kein Heap-Stress)
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, jsonString);
  
    if (error) {
        Serial.print("JSON Parse Error: ");
        Serial.println(error.c_str());
        return MaxError::BLE_PARSE_ERROR; // Fehlercode: Ungültiges JSON
    }

    // =========================================================
    // PHASE 1: VALIDIERUNG & EXTRAKTION (Transaction Prepare)
    // Wir sammeln erst alle Werte. Wenn IRGENDWAS falsch ist,
    // brechen wir ab, bevor wir den State ändern.
    // =========================================================

    // --- MODE ---
    MaxFanMode newMode;
    bool hasMode = false;
    if (doc.containsKey("mode")) {
        // 1. Typ-Check: Ist es ein String?
        if (!doc["mode"].is<const char*>()) {
            Serial.println("Err: 'mode' must be string"); 
            return MaxError::BLE_INVALID_MODE; 
        }
        // 2. Inhalt-Check: Ist der String gültig?
        const char* s = doc["mode"];
        if (!tryParseFanMode(s, newMode)) {
            Serial.print("Err: Invalid mode value: "); Serial.println(s);
            return MaxError::BLE_INVALID_MODE;
        }
        hasMode = true;
    }

    // --- COVER ---
    CoverState newCover;
    bool hasCover = false;
    if (doc.containsKey("cover")) {
        if (!doc["cover"].is<const char*>()) {
            Serial.println("Err: 'cover' must be string");
            return MaxError::BLE_INVALID_COVER;
        }
        const char* s = doc["cover"];
        if (!tryParseCoverState(s, newCover)) {
            Serial.print("Err: Invalid cover value: "); Serial.println(s);
            return MaxError::BLE_INVALID_COVER;
        }
        hasCover = true;
    }

    // --- AIRFLOW ---
    MaxFanDirection newAir;
    bool hasAir = false;
    if (doc.containsKey("airflow")) {
        if (!doc["airflow"].is<const char*>()) {
            return MaxError::BLE_INVALID_AIRFLOW;
        }
        const char* s = doc["airflow"];
        if (!tryParseFanDirection(s, newAir)) {
             return MaxError::BLE_INVALID_AIRFLOW;
        }
        hasAir = true;
    }

    // --- SPEED ---
    int newSpeed;
    bool hasSpeed = false;
    if (doc.containsKey("speed")) {
        // Nutzt den robusten Int-Helper (Min: 0, Max: 100)
        // Akzeptiert auch 50.0, lehnt aber Strings/"true" ab.
        if (!tryParseInt(doc["speed"], newSpeed, 0, 100)) {
            return MaxError::BLE_INVALID_SPEED;
        }
        hasSpeed = true;
    }

    // --- TEMPERATURE ---
    int newTemp;
    bool hasTemp = false;
    if (doc.containsKey("temperature")) {
        // Plausibilitätsbereich: -20°C bis 80°C
        if (!tryParseInt(doc["temperature"], newTemp, -20, 80)) {
            return MaxError::BLE_INVALID_TEMP;
        }
        hasTemp = true;
    }

    // =========================================================
    // PHASE 2: ANWENDUNG (Commit Transaction)
    // Wir sind hier sicher, dass ALLES validiert ist.
    // Jetzt darf der State geändert werden.
    // =========================================================

    if (hasMode)  SetMode(newMode);
    if (hasCover) SetCover(newCover);
    if (hasAir)   SetAirFlow(newAir);
    if (hasSpeed) SetSpeed(newSpeed);
    if (hasTemp)  SetTempCelsius(newTemp);

    return MaxError::NONE;
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
            this->stateByte &= ~MASK_FAN_ON;
            this->stateByte &= ~MASK_SPECIAL;
            this->stateByte &= ~MASK_COVER_OPEN;
            this->stateByte &= ~MASK_AUTO;
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
    this->stateByte &= (~MASK_COVER_OPEN);
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










