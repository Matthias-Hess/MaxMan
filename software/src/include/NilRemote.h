#ifndef NILREMOTE_H
#define NILREMOTE_H

#include "RemoteAccess.h"

class NilRemote : public RemoteAccess {
public:
    NilRemote() {}
    void begin(const char* deviceName = nullptr) override {}
    void setCommandCallback(CommandCallback cb) override { (void)cb; }
    void notifyStatus(const MaxFanState& state) override { (void)state; }
    void loop() override {}
    bool isConnected() override { return false; }
    Icon getIcon() override { return ICON_NONE; }
    char getIndicatorLetter() override { return '\0'; }
};

#endif
