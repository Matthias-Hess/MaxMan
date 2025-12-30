// AppMode.h
#pragma once

#include <U8g2lib.h>
#include "Encoder.h"
#include "ChordInput.h"

enum class ModeAction {
    NONE,
    SWITCH_TO_CONFIG,
    SWITCH_TO_STANDARD
};

class AppMode {
protected:
    U8G2& _display;          
    Encoder& _encoder;
    ChordInput& _buttons;

public:
    
    AppMode(U8G2& display, Encoder& encoder, ChordInput& buttons) 
        : _display(display), _encoder(encoder), _buttons(buttons) {}

    virtual ~AppMode() {}
    virtual void enter() = 0;
    virtual ModeAction loop() = 0;
};