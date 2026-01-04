#ifndef MAXFANCONFIG_H
#define MAXFANCONFIG_H

#include <Arduino.h>

// 1. Das dumme Daten-Objekt
struct ConfigData {
    int connection;          
    int blePin;
    char wifiPassword[64];
    int displayTimeoutSeconds;  
    char wifiSSID[64];
    // MQTT settings
    char mqttHost[64];
    int mqttPort;
    char mqttClientId[64];
    char mqttUsername[64];
    char mqttPassword[64];
    char mqttCommandTopic[64];
    char mqttStateTopic[64];
    bool mqttUseTls;

    bool operator==(const ConfigData& other) const {
        return (connection == other.connection) &&
               (blePin == other.blePin) &&
               (strncmp(wifiPassword, other.wifiPassword, 64) == 0) &&
               (displayTimeoutSeconds == other.displayTimeoutSeconds) &&
               (strncmp(wifiSSID, other.wifiSSID, 64) == 0) &&
               (strncmp(mqttHost, other.mqttHost, 64) == 0) &&
               (mqttPort == other.mqttPort) &&
               (strncmp(mqttClientId, other.mqttClientId, 64) == 0) &&
               (strncmp(mqttUsername, other.mqttUsername, 64) == 0) &&
               (strncmp(mqttPassword, other.mqttPassword, 64) == 0) &&
               (strncmp(mqttCommandTopic, other.mqttCommandTopic, 64) == 0) &&
               (strncmp(mqttStateTopic, other.mqttStateTopic, 64) == 0) &&
               (mqttUseTls == other.mqttUseTls);
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