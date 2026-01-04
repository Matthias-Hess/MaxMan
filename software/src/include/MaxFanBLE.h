#ifndef MAXFANBLE_H
#define MAXFANBLE_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Preferences.h>
#include <functional>
#include "MaxFanState.h"
#include "RemoteAccess.h"

class MaxFanBLE : public RemoteAccess {
public:
    MaxFanBLE();
    
    void begin(const char* deviceName = "MaxxFan Controller");
    void setCommandCallback(RemoteAccess::CommandCallback callback) override;
    void notifyStatus(const MaxFanState& currentState) override;
    bool isConnected() override;
    void loop() override;
    uint32_t getPin() const { return _pinCode; }

private:
    BLEServer* _pServer;
    BLECharacteristic* _pCommandChar;
    BLECharacteristic* _pStatusChar;
    MaxFanState _lastSentState; 
    bool _forceUpdate;
    CommandCallback _onCommandReceived;
    bool _deviceConnected;
    uint32_t _pinCode;

    // --- Interne Klassen ---
    class MyServerCallbacks : public BLEServerCallbacks {
        MaxFanBLE* _parent;
    public:
        MyServerCallbacks(MaxFanBLE* p) : _parent(p) {}
        void onConnect(BLEServer* s) override;
        void onDisconnect(BLEServer* s) override;
    };

    class MyCharCallbacks : public BLECharacteristicCallbacks {
        MaxFanBLE* _parent;
    public:
        MyCharCallbacks(MaxFanBLE* p) : _parent(p) {}
        void onWrite(BLECharacteristic* pChar) override;
    };

    // Security brauchen wir intern trotzdem, damit der PIN-Mechanismus greift,
    // aber ohne Kommunikation nach außen.
    class MySecurityCallbacks : public BLESecurityCallbacks {
    public:
        uint32_t onPassKeyRequest() override { return 0; }
        void onPassKeyNotify(uint32_t pass_key) override {}
        bool onConfirmPIN(uint32_t pass_key) override { return true; }
        bool onSecurityRequest() override { return true; }
        void onAuthenticationComplete(esp_ble_auth_cmpl_t cmpl) override {}
    };
};

#endif