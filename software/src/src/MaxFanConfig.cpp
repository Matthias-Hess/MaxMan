#include "MaxFanConfig.h"
#include <Preferences.h>
#include <BLEDevice.h>
#include "esp_gap_ble_api.h"

// Die ECHTE Instanz
ConfigData GlobalConfig;

// --- Interne Hilfsfunktion (private) ---
static void clearBondsInternal() {
    int dev_num = esp_ble_get_bond_device_num();
    if (dev_num == 0) return;
    
    Serial.println("ConfigManager: Lösche Bonds...");
    esp_ble_bond_dev_t *dev_list = (esp_ble_bond_dev_t *)malloc(sizeof(esp_ble_bond_dev_t) * dev_num);
    if (dev_list) {
        esp_ble_get_bond_device_list(&dev_num, dev_list);
        for (int i = 0; i < dev_num; i++) {
            esp_ble_remove_bond_device(dev_list[i].bd_addr);
        }
        free(dev_list);
    }
}

// --- Implementierung ConfigManager ---

void ConfigManager::load() {
    Preferences prefs;
    prefs.begin("config", true); // ReadOnly

    GlobalConfig.connection = prefs.getInt("connection", 0);
    GlobalConfig.blePin = prefs.getInt("blepin", 0);
    GlobalConfig.displayTimeoutSeconds = prefs.getInt("displayTimeoutS", 20);
    // Erster Start? -> PIN generieren
    if (GlobalConfig.blePin == 0) {
        prefs.end();
        prefs.begin("config", false); // Write
        GlobalConfig.blePin = (esp_random() % 900000) + 100000;
        prefs.putInt("blepin", GlobalConfig.blePin);
        prefs.end(); // Schließen und neu ReadOnly öffnen oder so lassen
        Serial.println("ConfigManager: Neuen PIN generiert.");
    }

    String pwd = prefs.getString("wifiPassword", "Start123");
    strncpy(GlobalConfig.wifiPassword, pwd.c_str(), 64);
    GlobalConfig.wifiPassword[63] = '\0'; // Safety

    String wifiSSID = prefs.getString("wifiSSID", "");
    strncpy(GlobalConfig.wifiSSID, wifiSSID.c_str(), 64);
    GlobalConfig.wifiSSID[63] = '\0'; // Safety

    prefs.end();
    Serial.println("ConfigManager: Config geladen.");
}

void ConfigManager::saveAndReboot(const ConfigData& newData) {
    Serial.println("ConfigManager: Speichere...");
    
    Preferences prefs;
    prefs.begin("config", false); // Write
    
    prefs.putInt("connection", newData.connection);
    prefs.putInt("blepin", newData.blePin);
    prefs.putInt("displayTimeoutS", newData.displayTimeoutSeconds);
    prefs.putString("wifiPassword", newData.wifiPassword);
    prefs.putString("wifiSSID", newData.wifiSSID);
    prefs.end();

    // Der intelligente Check: Wurde der PIN geändert?
    if (newData.blePin != GlobalConfig.blePin) {
        Serial.println("ConfigManager: PIN geändert -> Bonding Reset nötig.");
        // Wir müssen BLE kurz initieren, falls es aus ist, um Bonds zu löschen
        BLEDevice::init("TEMP_CLEAR"); 
        clearBondsInternal();
        delay(500); // Zeit für Flash
    }

    Serial.println("ConfigManager: Rebooting...");
    ESP.restart();
}