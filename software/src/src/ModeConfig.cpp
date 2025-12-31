#include "ModeConfig.h"
#include <Preferences.h>
#include <BLEDevice.h>
#include "esp_gap_ble_api.h"
// --- HILFSFUNKTION: Manuelles Löschen aller Bonds ---
// Da esp_ble_bond_dev_delete_all() auf dem C3 oft fehlt,
// iterieren wir durch die Liste und löschen einzeln.
void clearBonds() {
    int dev_num = esp_ble_get_bond_device_num();
    if (dev_num == 0) {
        Serial.println("BLE: Keine gespeicherten Bindungen gefunden.");
        return;
    }

    Serial.printf("BLE: Lösche %d gespeicherte Bindungen...\n", dev_num);

    // Speicher reservieren für die Liste der Geräte
    esp_ble_bond_dev_t *dev_list = (esp_ble_bond_dev_t *)malloc(sizeof(esp_ble_bond_dev_t) * dev_num);
    
    if (dev_list) {
        esp_ble_get_bond_device_list(&dev_num, dev_list);
        for (int i = 0; i < dev_num; i++) {
            esp_ble_remove_bond_device(dev_list[i].bd_addr);
        }
        free(dev_list); // Speicher freigeben
        Serial.println("BLE: Alle Bindungen gelöscht.");
    } else {
        Serial.println("BLE: Fehler bei Speicherreservierung (clearBonds).");
    }
}
// ----------------------------------------------------


static bool _mustExit;
Preferences prefs; 

ModeConfig* ModeConfig::instance = nullptr;
char ModeConfig::wifiPassword[GEM_STR_LEN] = "Start123";
int ModeConfig::blePin = 0;
int ModeConfig::connection = 0;

int originalBlePin=0;

SelectOptionInt optionsConnection[] = {
    {"None", 0},
    {"BLE",  1},
    {"WLAN", 2}
};

GEMSelect selectConnection(3, optionsConnection);


ModeConfig::ModeConfig(U8G2* display, Encoder* encoder, ChordInput* input)
    : 
     AppMode(*display, *encoder, *input), 
    _menu(*display, GEM_POINTER_ROW, GEM_ITEMS_COUNT_AUTO),
    _pageMain("Main Menu"),
    _itemExit("Exit Configuration", callbackExit),
    _itemGenerateNewPIN("Generate new PIN", callbackGenerateNewPIN),
    _itemPassword("WLAN PW:", wifiPassword),
    _itemConnection("Connection", connection, selectConnection),
    _itemBlePin("PIN", blePin)
{
    instance = this;
    

    // --- WICHTIG: Menü-Struktur nur EINMAL im Konstruktor aufbauen ---
    _pageMain.addMenuItem(_itemConnection);
    _pageMain.addMenuItem(_itemPassword);
    _pageMain.addMenuItem(_itemGenerateNewPIN);
    _pageMain.addMenuItem(_itemBlePin);
    _pageMain.addMenuItem(_itemExit);
    _itemPassword.setAdjustedASCIIOrder();
    

    load();
    originalBlePin = blePin;

    
    // Seite registrieren (optional, aber gut für GEM Struktur)
    _menu.setMenuPageCurrent(_pageMain);
}


void ModeConfig::enter() {
    _mustExit = false;
    _menu.setSplashDelay(0); // kein Splash Screen zeigen
    _menu.init();
    _menu.reInit();
    _menu.drawMenu();
}

ModeAction ModeConfig::loop() {

    bool actionDetected = false;
    if(ModeConfig::blePin <100000){
        ModeConfig::blePin +=100000;
        _menu.drawMenu();
    }
        

    if(_mustExit) return ModeAction::SWITCH_TO_STANDARD;

    if(connection==2 && _itemPassword.getHidden()){
        _itemPassword.show();
        actionDetected = true;
    }

    if(connection!=2 && !_itemPassword.getHidden()){
        _itemPassword.hide();
        actionDetected = true;
    }
        
    if(connection==1 && _itemGenerateNewPIN.getHidden()){
        _itemGenerateNewPIN.show();
        actionDetected = true;
    }

    if(connection!=1 && !_itemGenerateNewPIN.getHidden()){
        _itemGenerateNewPIN.hide();
        actionDetected = true;
    }
    
     if (!_menu.readyForKey()) return ModeAction::NONE;

    

    // Encoder
    int delta = _encoder.getDelta();
    if (delta != 0) {
        actionDetected = true;
        if(_buttons.IsKeyDown(ENCODER_BUTTON)){
             _buttons.CancelCurrentChord();
             if (delta >0){
                
                _menu.registerKeyPress(GEM_KEY_LEFT);
             }  else {
                
                _menu.registerKeyPress(GEM_KEY_RIGHT);
             }
        } else {
            if (delta > 0) {
                
                _menu.registerKeyPress(GEM_KEY_DOWN);
            } else {
                
                _menu.registerKeyPress(GEM_KEY_UP);
            }
        }
    }

    // Buttons
    _buttons.tick();
    if (_buttons.hasEvent()) {
        KeyEvent evt = _buttons.popEvent();
        
        if (evt.IsSingle(ENCODER_BUTTON)) {
            
            _menu.registerKeyPress(GEM_KEY_OK);
            actionDetected = true;
        }
        if (evt.IsSingle(MODE_BUTTON)) {
            
            _menu.registerKeyPress(GEM_KEY_CANCEL);
            actionDetected = true;
        }
    }

    if (actionDetected) {
        _menu.drawMenu();
    }
    return ModeAction::NONE;
}


void ModeConfig::callbackExit() {
    instance->_display.clearBuffer();
    instance->_display.sendBuffer();
    save();
    ESP.restart();
}
void ModeConfig::callbackGenerateNewPIN() {
    ModeConfig::blePin = (esp_random() % 900000) + 100000; // gespeichert wird später
}
void ModeConfig::callbackPassword(GEMCallbackData data) {
    
}

void ModeConfig::save() {
    prefs.begin("config", false); 

    // 2. Werte schreiben
    prefs.putInt("connection", ModeConfig::connection);
    prefs.putInt("blepin", ModeConfig::blePin);
    prefs.putString("wifiPassword", ModeConfig::wifiPassword);
    // 3. Schließen
    prefs.end();
    
    if(originalBlePin != blePin){
       Serial.println("PIN wurde geändert. Starte Bonding-Löschung...");
       
       // Damit wir Bonds löschen können, muss der BLE Stack initialisiert sein.
       // Wir starten ihn kurz "dummy", falls er nicht läuft.
       BLEDevice::init("TEMP_CLEAR"); 
       
       // Manuelle Löschung aufrufen
       clearBonds();
       
       // WICHTIG: Kurze Pause für Flash-Vorgänge
       delay(500); 
    }
}

void ModeConfig::load() {
    prefs.begin("config", false); // true = ReadOnly (sicherer)

    
    ModeConfig::connection = prefs.getInt("connection", 0);
    ModeConfig::blePin = prefs.getInt("blepin",0);

    if(ModeConfig::blePin==0){
        Serial.println("Kein BLE PIN vorhanden, ich mache einen...");
        ModeConfig::blePin = (esp_random() % 900000) + 100000;
        
        // Speichern für den nächsten Start
        prefs.putInt("blepin", ModeConfig::blePin);
    }

    String tempPassword = prefs.getString("wifiPassword", "yourPassword");
    strncpy(wifiPassword, tempPassword.c_str(), GEM_STR_LEN);
    wifiPassword[GEM_STR_LEN - 1] = '\0';

    prefs.end();
}