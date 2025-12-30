#ifndef MODE_CONFIG_H
#define MODE_CONFIG_H

#include <U8g2lib.h>
#include <GEM_u8g2.h>
#include "AppMode.h"          // WICHTIG: Basisklasse
#include "Encoder.h"
#include "ChordInput.h"
#include "MaxFanConstants.h"

// WICHTIG: Erbt von AppMode
class ModeConfig : public AppMode {
public:
    ModeConfig(U8G2* display, Encoder* encoder, ChordInput* input);

    void enter() override;
    ModeAction loop() override;


private:
    static char wifiPassword[GEM_STR_LEN];
    static int connection; // 0 = None, 1 = BLE, 2 = WLAN
    GEM_u8g2 _menu;
    GEMPage _pageMain;
    GEMItem _itemExit;
    GEMItem _itemPair;
    GEMItem _itemPassword; 
    GEMItem _itemConnection;
    static void callbackExit();
    static void callbackPair();
    static void save();
    static void load();
    static ModeConfig* instance;
    static void callbackPassword(GEMCallbackData data);

};

#endif