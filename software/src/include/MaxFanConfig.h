#ifndef MAXFANCONFIG_H
#define MAXFANCONFIG_H

#include <Arduino.h>

// 1. Das dumme Daten-Objekt
struct ConfigData {
    int connection;          
    int blePin;
    char wifiPassword[64];
    int displayTimeoutSeconds;  
    char wifiSID[64];

    bool operator==(const ConfigData& other) const {
        return (connection == other.connection) &&
               (blePin == other.blePin) &&
               (strncmp(wifiPassword, other.wifiPassword, 64) == 0) &&
               (displayTimeoutSeconds == other.displayTimeoutSeconds);
    }
    
    bool operator!=(const ConfigData& other) const {
        return !(*this == other);
    }
};

// Die globale Instanz (überall lesbar)
extern ConfigData GlobalConfig; 

// 2. Der schlaue Manager (statisch)
class ConfigManager {
public:
    // Lädt Flash -> GlobalConfig
    static void load(); 
    
    // Speichert newData -> Flash und kümmert sich um Bonds & Reboot
    static void saveAndReboot(const ConfigData& newData);
};

#endif