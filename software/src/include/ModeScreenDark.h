#pragma once

#include "AppMode.h"
#include <MaxFanState.h>
#include <MaxRemote.h>
#include <MaxReceiver.h>
#include "FanController.h"

class ModeScreenDark : public AppMode {
private:
    MaxFanState& _state;
    MaxRemote& _remote;
    MaxReceiver& _irReceiver;
    FanController& _remoteAccess;

public:
    ModeScreenDark(U8G2& u8g2, Encoder& enc, ChordInput& btns,
                   MaxFanState& state, MaxRemote& remote, 
                   MaxReceiver& irReceiver, FanController& remoteAccess);

    virtual void enter() override;
    virtual ModeAction loop() override;
};

