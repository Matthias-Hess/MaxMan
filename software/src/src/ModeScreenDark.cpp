#include "ModeScreenDark.h"
#include <U8g2lib.h>

ModeScreenDark::ModeScreenDark(U8G2& u8g2, Encoder& enc, ChordInput& btns,
                                                             MaxFanState& state, MaxRemote& remote, 
                                                             MaxReceiver& irReceiver, FanController& remoteAccess)
        : AppMode(u8g2, enc, btns),
            _state(state), _remote(remote), _irReceiver(irReceiver), _remoteAccess(remoteAccess)
{
}

void ModeScreenDark::enter() {
    Serial.println(F("Entering Screen Dark Mode"));
    
    // Blank the screen - clear buffer and send empty buffer
    _display.clearBuffer();
    _display.sendBuffer();
    
    // Turn off display power (for OLED displays)
    _display.setPowerSave(1);
}

ModeAction ModeScreenDark::loop() {
    bool isConnected = _remoteAccess.isConnected();

    if(isConnected){
        _remoteAccess.notifyStatus(_state);
    }

    _remote.send(_state);
    _irReceiver.update(_state);

    // Check for any encoder movement
    int delta = _encoder.getDelta();
    if (delta != 0) {
        // Consume the input and return to standard mode
        Serial.println(F("ScreenDark: Encoder input detected, returning to Standard Mode"));
        _display.setPowerSave(0); // Turn display back on
        return ModeAction::SWITCH_TO_STANDARD;
    }
    
    // Check for any button events
    if (_buttons.hasEvent()) {
        // Consume the event (pop it but don't process)
        _buttons.popEvent();
        Serial.println(F("ScreenDark: Button input detected, returning to Standard Mode"));
        _display.setPowerSave(0); // Turn display back on
        return ModeAction::SWITCH_TO_STANDARD;
    }
    
    return ModeAction::NONE;
}

