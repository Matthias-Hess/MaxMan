#include "MaxErrors.h"

const char* getMaxErrorText(MaxError err) {
    switch(err) {
        // BLE Errors
        case MaxError::BLE_PARSE_ERROR:       return "JSON Format Err";
        case MaxError::BLE_INVALID_MODE:      return "Invalid Mode";
        case MaxError::BLE_INVALID_COVER:     return "Invalid Cover";
        case MaxError::BLE_INVALID_AIRFLOW:   return "Invalid Airflow";
        case MaxError::BLE_INVALID_SPEED:     return "Invalid Speed";
        case MaxError::BLE_INVALID_TEMP:      return "Invalid Temp";
        
        
        
        case MaxError::NONE:                  return "No Error";
        default:                              return "System Fault";
    }
}

const char* getMaxErrorCaption(MaxError err) {
    switch(err) {
        // BLE Errors
        case MaxError::BLE_PARSE_ERROR:       return "JSON Error";
        case MaxError::BLE_INVALID_MODE:      return "JSON Error";
        case MaxError::BLE_INVALID_COVER:     return "JSON Error";
        case MaxError::BLE_INVALID_AIRFLOW:   return "JSON Error";
        case MaxError::BLE_INVALID_SPEED:     return "JSON Error";
        case MaxError::BLE_INVALID_TEMP:      return "JSON Error";
        
        
        
        case MaxError::NONE:                  return "No Error";
        default:                              return "Unknown Error";
    }
}