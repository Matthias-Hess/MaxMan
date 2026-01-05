#ifndef FANCONTROLLER_H
#define FANCONTROLLER_H

#include <Arduino.h>
#include <functional>
#include "MaxFanState.h"

class FanController {
public:
    typedef std::function<void(const String&)> CommandCallback;
    virtual ~FanController() {}
    virtual void begin(const char* deviceName = nullptr) = 0;
    virtual void setCommandCallback(CommandCallback cb) = 0;
    virtual void notifyStatus(const MaxFanState& state) = 0;
    virtual void loop() = 0;
    virtual bool isConnected() = 0;
    // Icon type for display
    enum Icon { ICON_NONE = 0, ICON_BLE = 1, ICON_MQTT = 2, ICON_TIMER = 3 };

    // Returns the icon to use for display.
    virtual Icon getIcon() = 0;
    // Returns a single-character indicator when connection is partial (e.g. 'W','R','C','B'), or '\0' when none.
    virtual char getIndicatorLetter() = 0;
};

#endif
