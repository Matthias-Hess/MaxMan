#ifndef MODE_CONFIG_H
#define MODE_CONFIG_H

#include <U8g2lib.h>
#include <GEM_u8g2.h>
#include "AppMode.h"          // WICHTIG: Basisklasse
#include "Encoder.h"
#include "ChordInput.h"
#include "MaxFanConstants.h"
#include "MaxFanConfig.h"

// WICHTIG: Erbt von AppMode
class ModeConfig : public AppMode {
public:
    ModeConfig(U8G2* display, Encoder* encoder, ChordInput* input);

    void enter() override;
    ModeAction loop() override;


private:
    bool _mustExit;
    GEM_u8g2 _menu;
    GEMPage _pageMain;
    GEMItem _itemExit;
    GEMItem _itemGenerateNewPIN;
    GEMItem _itemPassword; 
    GEMItem _itemConnection;
    GEMItem _itemBlePin;
    GEMItem _itemDisplayTimeoutSeconds;
    // 1. Die neue Unter-Seite
    GEMPage _pageExit; 

    // 2. Die Items für das Exit-Menü
    GEMItem _itemSave;
    GEMItem _itemDiscard;
    GEMItem _itemBack;

    ConfigData _editConfig;

    // 3. Statische Callbacks für die neuen Funktionen
    static void callbackSaveAndRestart();
    static void callbackDiscardAndRestart();
    static void callbackGoBack(); // Um manuell zurückzuspringen
    static void callbackCheckExit(); 
    static void callbackExit();
    static void callbackGenerateNewPIN();
    static void save();
    static void load();
    static ModeConfig* instance;
    static void callbackPassword(GEMCallbackData data);

};

#endif