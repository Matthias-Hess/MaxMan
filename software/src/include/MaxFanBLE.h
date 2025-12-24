#ifndef MAXFANBLE_H
#define MAXFANBLE_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <functional>

class MaxFanBLE {
public:
    // Ein einfacher Callback, der nur den rohen String liefert
    typedef std::function<void(const String&)> CommandCallback;

    MaxFanBLE();
    
    // Startet den Server und die Advertising
    void begin(const char* deviceName = "MaxxFan Controller");
    
    // Hier registriert der Main-Loop seine Logik
    void setCommandCallback(CommandCallback callback);
    
    // Pusht den aktuellen Zustand (als JSON-String) an das Smartphone
    void notifyStatus(const String& jsonStatus);

    bool isConnected();

private:
    BLEServer* _pServer;
    BLECharacteristic* _pCommandChar;
    BLECharacteristic* _pStatusChar;
    CommandCallback _onCommandReceived;
    bool _deviceConnected = false;

    // Interne Klassen für die BLE-Events
    class MyServerCallbacks : public BLEServerCallbacks {
        MaxFanBLE* _parent;
    public:
        MyServerCallbacks(MaxFanBLE* p) : _parent(p) {}
        void onConnect(BLEServer* s) override { _parent->_deviceConnected = true; }
        void onDisconnect(BLEServer* s) override { 
            _parent->_deviceConnected = false;
            BLEDevice::startAdvertising(); // Sofort wieder sichtbar sein
        }
    };

    class MyCharCallbacks : public BLECharacteristicCallbacks {
        MaxFanBLE* _parent;
    public:
        MyCharCallbacks(MaxFanBLE* p) : _parent(p) {}
        void onWrite(BLECharacteristic* pChar) override;
    };
};

#endif