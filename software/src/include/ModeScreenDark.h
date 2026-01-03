#pragma once

#include "AppMode.h"

class ModeScreenDark : public AppMode {
public:
    ModeScreenDark(U8G2& u8g2, Encoder& enc, ChordInput& btns);

    virtual void enter() override;
    virtual ModeAction loop() override;
};

