#ifndef REMOTEACCESS_H
#define REMOTEACCESS_H

#include <Arduino.h>
#include <functional>
#include "MaxFanState.h"

class RemoteAccess {
public:
    typedef std::function<void(const String&)> CommandCallback;
    virtual ~RemoteAccess() {}
    virtual void begin(const char* deviceName = nullptr) = 0;
    virtual void setCommandCallback(CommandCallback cb) = 0;
    virtual void notifyStatus(const MaxFanState& state) = 0;
    virtual void loop() = 0;
    virtual bool isConnected() = 0;
};

#endif
