#include "MaxFanBLE.h"
#include "MaxFanConfig.h"
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define COMMAND_UUID        "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define STATUS_UUID         "cba1d466-344c-4be3-ab3f-1890d5c0c0c0"

MaxFanBLE::MaxFanBLE() 
  : _pServer(nullptr), _pCommandChar(nullptr), _pStatusChar(nullptr), 
    _onCommandReceived(nullptr), _deviceConnected(false), _pinCode(0),
    _forceUpdate(true) // Starten mit erzwungenem Update
{
}

void MaxFanBLE::begin(const char* deviceName) {
    // 1. Initialisierung
    BLEDevice::init(deviceName);

    // 2. PIN laden
    _pinCode = GlobalConfig.blePin;
    
    Serial.printf("BLE: Security PIN ist %d\n", _pinCode);

    // 3. Security Einstellungen (Still notwendig für PIN-Abfrage am Handy)
    BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT_MITM);
    BLEDevice::setSecurityCallbacks(new MySecurityCallbacks());

    BLESecurity *pSecurity = new BLESecurity();
    pSecurity->setStaticPIN(_pinCode); 
    pSecurity->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_MITM_BOND);
    // IO_CAP_OUT signalisiert dem Handy: "Ich zeige dir was an (den statischen PIN), tipp ihn ein."
    pSecurity->setCapability(ESP_IO_CAP_OUT); 
    pSecurity->setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
    pSecurity->setRespEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);

    // 4. Server & Service
    _pServer = BLEDevice::createServer();
    _pServer->setCallbacks(new MyServerCallbacks(this));
    
    BLEService* pService = _pServer->createService(SERVICE_UUID);
    
    // 5. Characteristics (Verschlüsselt)
    _pCommandChar = pService->createCharacteristic(COMMAND_UUID, BLECharacteristic::PROPERTY_WRITE);
    _pCommandChar->setAccessPermissions(ESP_GATT_PERM_WRITE_ENC_MITM); 
    _pCommandChar->setCallbacks(new MyCharCallbacks(this));
    
    _pStatusChar = pService->createCharacteristic(STATUS_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
    _pStatusChar->setAccessPermissions(ESP_GATT_PERM_READ_ENC_MITM);
    _pStatusChar->addDescriptor(new BLE2902());
    
    pService->start();
    
    // 6. Advertising
    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06); 
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();

    Serial.println("BLE bereit (Sicherer Modus).");
}

void MaxFanBLE::setCommandCallback(CommandCallback callback) {
    _onCommandReceived = callback;
}

void MaxFanBLE::notifyStatus(const MaxFanState& currentState) {
    // 1. Wenn keiner zuhört, sofort raus
    if (!_deviceConnected || !_pStatusChar) {
        return;
    }

    // 2. STROMSPAR-CHECK:
    // Nutzt den effizienten == Operator von MaxFanState.
    // Wenn Zustand gleich wie vorher UND kein erzwungenes Update -> Raus!
    if ((_lastSentState == currentState) && !_forceUpdate) {
        return; 
    }

    // 3. Es hat sich was geändert (oder neuer Client):
    _lastSentState = currentState; // Zustand merken (Byte-Kopie)
    _forceUpdate = false;          // Flag zurücksetzen

    // Erst JETZT den String für das Handy bauen
    String jsonStatus = currentState.ToJson(); 
    
    // Senden
    _pStatusChar->setValue(jsonStatus.c_str());
    _pStatusChar->notify();
    Serial.println("Notified CLient");
}

bool MaxFanBLE::isConnected() {
    return _deviceConnected;
}

// --- Callbacks ---

void MaxFanBLE::MyServerCallbacks::onConnect(BLEServer* s) {
    _parent->_deviceConnected = true;
    // WICHTIG: Wenn sich das Handy neu verbindet, kennt es den aktuellen Status nicht.
    // Wir zwingen notifyStatus() dazu, beim nächsten Mal zu senden,
    // auch wenn sich der State technisch nicht geändert hat.
    _parent->_forceUpdate = true;
    Serial.println("BLE: Client verbunden.");
}

void MaxFanBLE::MyServerCallbacks::onDisconnect(BLEServer* s) {
    _parent->_deviceConnected = false;
    Serial.println("BLE: Client getrennt.");
    BLEDevice::startAdvertising(); 
}

void MaxFanBLE::MyCharCallbacks::onWrite(BLECharacteristic* pChar) {
    std::string rxValue = pChar->getValue();
    if (rxValue.length() > 0 && _parent->_onCommandReceived) {
        _parent->_onCommandReceived(String(rxValue.c_str()));
    }
}

void MaxFanBLE::loop() {
    // BLE server runs in background; nothing to do each loop for now
}