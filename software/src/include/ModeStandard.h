#pragma once

#include "AppMode.h"
#include <MaxFanState.h>
#include <MaxFanDisplay.h>
#include <MaxRemote.h>
#include <MaxReceiver.h>
#include <MaxFanBLE.h>

class ModeStandard : public AppMode {
private:
    // Referenzen auf die spezifischen Objekte, die dieser Mode braucht
    MaxFanState& _state;
    MaxFanDisplay& _display;
    MaxRemote& _remote;
    MaxReceiver& _irReceiver;
    MaxFanBLE& _ble;

    // Lokale Variablen für Logik, die früher static oder global war
    bool _wasConnected = false;
    long _testValue = 0; 

public:
    // Der Konstruktor bekommt alles injiziert
    ModeStandard(U8G2& u8g2, Encoder& enc, ChordInput& btns, 
                 MaxFanState& state, MaxFanDisplay& display, 
                 MaxRemote& remote, MaxReceiver& irReceiver, MaxFanBLE& ble)
        : AppMode(u8g2, enc, btns), // HIER: Wir reichen u8g2 an die Basisklasse weiter
          _state(state), _display(display), _remote(remote), 
          _irReceiver(irReceiver), _ble(ble) 
    {}

    virtual void enter() override;
    virtual ModeAction loop() override;
};