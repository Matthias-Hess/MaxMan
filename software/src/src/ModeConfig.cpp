#include "ModeConfig.h"
#include "esp_gap_ble_api.h"

// --- Hilfsfunktion clearBonds bleibt gleich ---
void clearBonds() {
    int dev_num = esp_ble_get_bond_device_num();
    if (dev_num == 0) return;
    esp_ble_bond_dev_t *dev_list = (esp_ble_bond_dev_t *)malloc(sizeof(esp_ble_bond_dev_t) * dev_num);
    if (dev_list) {
        esp_ble_get_bond_device_list(&dev_num, dev_list);
        for (int i = 0; i < dev_num; i++) {
            esp_ble_remove_bond_device(dev_list[i].bd_addr);
        }
        free(dev_list);
    }
}
// ----------------------------------------------------

static bool _mustExit;


ModeConfig* ModeConfig::instance = nullptr;

SelectOptionInt optionsConnection[] = {
    {"None", 0},
    {"BLE",  1},
    {"WLAN", 2}
};

SelectOptionInt optionsTimeout[] = {
    {"Never", 0},
    {"10s",  10},
    {"20s", 20},
    {"30s",  30},
    {"1min",  60},
    {"5min",  300}
};

GEMSelect selectConnection(3, optionsConnection);
GEMSelect selectTimeout(6, optionsTimeout);

ModeConfig::ModeConfig(U8G2* display, Encoder* encoder, ChordInput* input)
    : 
     AppMode(*display, *encoder, *input), 
    _menu(*display, GEM_POINTER_ROW, GEM_ITEMS_COUNT_AUTO),
    _mustExit(false),
    
    // Hauptseite
    _pageMain("Main Menu"),

    // Die Exit-Seite (Titel des Untermenüs)
    _pageExit("Exit Menu"), 

    // Hauptmenü Items
    _itemConnection("Connection", _editConfig.connection, selectConnection),
    _itemPassword("WLAN PW:", _editConfig.wifiPassword),
    _itemGenerateNewPIN("Generate new PIN", callbackGenerateNewPIN),
    _itemBlePin("PIN", _editConfig.blePin),
    _itemDisplayTimeoutSeconds("Display off after", _editConfig.displayTimeoutSeconds, selectTimeout),

    // WICHTIG: Das Exit Item verweist jetzt auf die Page _pageExit, nicht mehr auf einen Callback!
    _itemExit("Exit Configuration", callbackCheckExit),

    // Die Items für das Exit Menü
    _itemSave("Save Changes", callbackSaveAndRestart),
    _itemDiscard("Discard Changes", callbackDiscardAndRestart),
    _itemBack("Go Back", callbackGoBack)
{
    instance = this;

    // --- 1. Aufbau des Hauptmenüs ---
    _pageMain.addMenuItem(_itemConnection);
    _pageMain.addMenuItem(_itemPassword);
    _pageMain.addMenuItem(_itemGenerateNewPIN);
    _pageMain.addMenuItem(_itemBlePin);
    _pageMain.addMenuItem(_itemDisplayTimeoutSeconds);
    _pageMain.addMenuItem(_itemExit); // Das öffnet jetzt das Untermenü
    
    _itemPassword.setAdjustedASCIIOrder();

    // --- 2. Aufbau des Exit-Untermenüs ---
    _pageExit.addMenuItem(_itemSave);
    _pageExit.addMenuItem(_itemDiscard);
    _pageExit.addMenuItem(_itemBack);

    
    
    // Menü initialisieren
    _menu.setMenuPageCurrent(_pageMain);
}

void ModeConfig::enter() {
    _editConfig = GlobalConfig; // Arbeits-KOPIE erstellen
    _mustExit = false;
    _menu.setMenuPageCurrent(_pageMain);
    _pageMain.setCurrentMenuItemIndex(0);
    _menu.setSplashDelay(0);
    _menu.init();
    _menu.reInit();
    _menu.drawMenu();
}

ModeAction ModeConfig::loop() {
    bool actionDetected = false;

    // Hack um PIN nur anzuzeigen/ändern
    while(_editConfig.blePin < 100000){
        _editConfig.blePin += 100000;
        _menu.drawMenu(); // Redraw nötig, falls Wert geändert wurde
    }
        
    if(_mustExit) return ModeAction::SWITCH_TO_STANDARD;

    // Logik zum Ein-/Ausblenden von Menüpunkten
    if(_editConfig.connection==2 && _itemPassword.getHidden()){
        _itemPassword.show();
        actionDetected = true;
    }
    if(_editConfig.connection!=2 && !_itemPassword.getHidden()){
        _itemPassword.hide();
        actionDetected = true;
    } 
    if(_editConfig.connection==1 && _itemGenerateNewPIN.getHidden()){
        _itemGenerateNewPIN.show();
        actionDetected = true;
    }
    if(_editConfig.connection!=1 && !_itemGenerateNewPIN.getHidden()){
        _itemGenerateNewPIN.hide();
        actionDetected = true;
    }
    
    // --- Input Handling ---
    if (_menu.readyForKey()) {
        int delta = _encoder.getDelta();
        if (delta != 0) {
            actionDetected = true;
            if(_buttons.IsKeyDown(ENCODER_BUTTON)){
                 _buttons.CancelCurrentChord();
                 if (delta > 0) _menu.registerKeyPress(GEM_KEY_LEFT);
                 else           _menu.registerKeyPress(GEM_KEY_RIGHT);
            } else {
                if (delta > 0) _menu.registerKeyPress(GEM_KEY_UP);
                else           _menu.registerKeyPress(GEM_KEY_DOWN);
            }
        }

        _buttons.tick();
        if (_buttons.hasEvent()) {
            KeyEvent evt = _buttons.popEvent();
            
            if (evt.IsSingle(ENCODER_BUTTON)) {
                _menu.registerKeyPress(GEM_KEY_OK);
                actionDetected = true;
            }
            // Optional: MODE Button als "Zurück" Taste nutzen
            if (evt.IsSingle(MODE_BUTTON)) {
                _menu.registerKeyPress(GEM_KEY_CANCEL); // GEM geht automatisch eine Ebene zurück
                actionDetected = true;
            }
        }
    }

    if (actionDetected) {
        _menu.drawMenu();
    }
    return ModeAction::NONE;
}

// ------------------------------------------------
// CALLBACKS
// ------------------------------------------------
void ModeConfig::callbackCheckExit() {
    // A. Vergleich: Haben wir was verändert?
    // Da wir den operator== im struct haben, ist das ein Einzeiler:
    if (instance->_editConfig == GlobalConfig) {
        
        // Fall 1: KEINE Änderungen
        Serial.println("Config: Nichts geändert. Gehe direkt raus.");
        instance->_mustExit = true; // Signalisiert dem loop(), dass wir fertig sind
        
    } else {
        
        // Fall 2: Änderungen erkannt
        Serial.println("Config: Änderungen! Zeige Save/Discard Menü.");
        
        // Manuelle Navigation zur Unterseite!
        instance->_menu.setMenuPageCurrent(instance->_pageExit);
        instance->_menu.drawMenu();
    }
}

// 1. Speichern und Neustart
void ModeConfig::callbackSaveAndRestart() {
    instance->_display.clearBuffer();
    instance->_display.drawStr(10, 30, "Saving...");
    instance->_display.sendBuffer();
    ConfigManager::saveAndReboot(instance->_editConfig);
}

// 2. Verwerfen und Neustart (Neustart ist am sichersten, um alte Werte zu laden)
void ModeConfig::callbackDiscardAndRestart() {
    instance->_mustExit = true;
}

// 3. Zurück zum Hauptmenü
void ModeConfig::callbackGoBack() {
    // GEM Methode um manuell die Seite zu wechseln
    instance->_menu.setMenuPageCurrent(instance->_pageMain);
    instance->_menu.drawMenu();
}

// --- Existierende Helper ---

void ModeConfig::callbackGenerateNewPIN() {
    instance->_editConfig.blePin = (esp_random() % 900000) + 100000;
}



