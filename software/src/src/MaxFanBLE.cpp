#include "MaxFanBLE.h"

// UUIDs (beibehalten aus deinem vorherigen Entwurf)
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define COMMAND_UUID        "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define STATUS_UUID         "cba1d466-344c-4be3-ab3f-1890d5c0c0c0"

MaxFanBLE::MaxFanBLE() 
  : _pServer(nullptr), _pCommandChar(nullptr), _pStatusChar(nullptr), 
    _onCommandReceived(nullptr), _deviceConnected(false) {
}

void MaxFanBLE::begin(const char* deviceName) {
    // 1. BLE Device Initialisierung
    BLEDevice::init(deviceName);
    
    // 2. Server erstellen
    _pServer = BLEDevice::createServer();
    _pServer->setCallbacks(new MyServerCallbacks(this));
    
    // 3. Service erstellen
    BLEService* pService = _pServer->createService(SERVICE_UUID);
    
    // 4. Command Characteristic (Vom Handy zum XIAO - WRITE)
    _pCommandChar = pService->createCharacteristic(
        COMMAND_UUID,
        BLECharacteristic::PROPERTY_WRITE
    );
    _pCommandChar->setCallbacks(new MyCharCallbacks(this));
    
    // 5. Status Characteristic (Vom XIAO zum Handy - NOTIFY/READ)
    _pStatusChar = pService->createCharacteristic(
        STATUS_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );
    // Notwendig, damit das Handy auf Notifications "hören" kann
    _pStatusChar->addDescriptor(new BLE2902());
    
    // 6. Service starten
    pService->start();
    
    // 7. Advertising (Sichtbarkeit) starten
    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(false);
    pAdvertising->setMinPreferred(0x06); // Hilft bei der Verbindungskompatibilität
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();

    Serial.println("BLE Transport-Layer aktiv: Warte auf Verbindung...");
}

void MaxFanBLE::setCommandCallback(CommandCallback callback) {
    _onCommandReceived = callback;
}

void MaxFanBLE::notifyStatus(const String& jsonStatus) {
    if (_deviceConnected && _pStatusChar) {
        _pStatusChar->setValue(jsonStatus.c_str());
        _pStatusChar->notify();
    }
}

bool MaxFanBLE::isConnected() {
    return _deviceConnected;
}

// --- Callback Implementierungen ---

void MaxFanBLE::MyCharCallbacks::onWrite(BLECharacteristic* pChar) {
    // Rohen Wert aus der Characteristic lesen
    std::string rxValue = pChar->getValue();
    
    if (rxValue.length() > 0 && _parent->_onCommandReceived) {
        // String konvertieren und direkt an den registrierten Callback im Main weitergeben
        _parent->_onCommandReceived(String(rxValue.c_str()));
    }
}