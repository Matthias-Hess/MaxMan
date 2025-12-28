#ifndef MAXERRORS_H
#define MAXERRORS_H

#include <Arduino.h>

// Das globale Error-Enum
enum class MaxError {
    NONE = 0,
    
    // --- BLE / JSON INPUT ERRORS (10-19) ---
    BLE_PARSE_ERROR = 10,
    BLE_INVALID_MODE,
    BLE_INVALID_COVER,
    BLE_INVALID_AIRFLOW,
    BLE_INVALID_SPEED,
    BLE_INVALID_TEMP

         
};

// Deklaration der Hilfsfunktion (Implementierung in .cpp)
const char* getMaxErrorText(MaxError err);
const char* getMaxErrorCaption(MaxError err);

#endif