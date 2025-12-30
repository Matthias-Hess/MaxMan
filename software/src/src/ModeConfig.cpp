#include "ModeConfig.h"
#include <Preferences.h>

static bool _mustExit;
Preferences prefs; 

ModeConfig* ModeConfig::instance = nullptr;
char ModeConfig::wifiPassword[GEM_STR_LEN] = "Start123";
int ModeConfig::connection = 0;

SelectOptionInt optionsConnection[] = {
    {"None", 0},
    {"BLE",  1},
    {"WLAN", 2}
};

GEMSelect selectConnection(3, optionsConnection);


ModeConfig::ModeConfig(U8G2* display, Encoder* encoder, ChordInput* input)
    : 
    // --- HIER IST DER FIX ---
    // Wir rufen den Konstruktor der Basisklasse auf und geben die Hardware weiter.
    // Da 'display' hier ein Zeiger ist, 'AppMode' aber eine Referenz will, schreiben wir *display.
    AppMode(*display, *encoder, *input), 
    // ------------------------

    // Jetzt initialisieren wir die GEM Objekte
    // (Achtung: Wir nutzen hier *display, da wir den Zeiger dereferenzieren müssen)
    _menu(*display, GEM_POINTER_ROW, GEM_ITEMS_COUNT_AUTO),
    _pageMain("Main Menu"),
    _itemExit("Exit Configuration", callbackExit),
    _itemPair("Pair", callbackPair),
    _itemPassword("WLAN PW:", wifiPassword),
    _itemConnection("Connection", connection, selectConnection)
{
    instance = this;

    // --- WICHTIG: Menü-Struktur nur EINMAL im Konstruktor aufbauen ---
    _pageMain.addMenuItem(_itemConnection);
    _pageMain.addMenuItem(_itemPassword);
    _pageMain.addMenuItem(_itemPair);
    _pageMain.addMenuItem(_itemExit);
    _itemPassword.setAdjustedASCIIOrder();

    load();

    
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

    if(_mustExit) return ModeAction::SWITCH_TO_STANDARD;

    if(connection==2 && _itemPassword.getHidden()){
        _itemPassword.show();
        actionDetected = true;
    }

    if(connection!=2 && !_itemPassword.getHidden()){
        _itemPassword.hide();
        actionDetected = true;
    }
        
    if(connection==1 && _itemPair.getHidden()){
        _itemPair.show();
        actionDetected = true;
    }

    if(connection!=1 && !_itemPair.getHidden()){
        _itemPair.hide();
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
void ModeConfig::callbackPair() {
    
}
void ModeConfig::callbackPassword(GEMCallbackData data) {
    Serial.println(F("--- PASSWORD CALLBACK ---"));
    
    // Test 1: Statische Variable direkt lesen
    Serial.print(F("Wert in static _wifiPassword: "));
    Serial.println(wifiPassword);

    // Test 2: Checken ob GEM Pointer jetzt gültig ist
    if (data.valChar != nullptr) {
        Serial.print(F("Wert von GEM (valChar): "));
        Serial.println(data.valChar);
    } else {
        Serial.println(F("GEM Pointer ist immer noch NULL!"));
    }
}

void ModeConfig::save() {
    // 1. Namespace öffnen (Name max 15 Zeichen!, false = Read/Write)
    prefs.begin("config", false); 

    // 2. Werte schreiben
    prefs.putInt("connection", ModeConfig::connection);
    prefs.putString("wifiPassword", ModeConfig::wifiPassword);
    

    // 3. Schließen
    prefs.end();
}

void ModeConfig::load() {
    prefs.begin("config", true); // true = ReadOnly (sicherer)

    
    ModeConfig::connection = prefs.getInt("connection", 0);

    String tempPassword = prefs.getString("wifiPassword", "yourPassword");
    strncpy(wifiPassword, tempPassword.c_str(), GEM_STR_LEN);
    wifiPassword[GEM_STR_LEN - 1] = '\0';

    prefs.end();
}