#ifndef TIMERVENTILATIONCONTROLLER_H
#define TIMERVENTILATIONCONTROLLER_H

#include <Arduino.h>
#include "FanController.h"
#include "MaxFanState.h"

class TimerVentilationController : public FanController {
public:
    TimerVentilationController();
    void begin(const char* deviceName = nullptr) override;
    void setCommandCallback(CommandCallback cb) override;
    void notifyStatus(const MaxFanState& state) override;
    void loop() override;
    bool isConnected() override;
    Icon getIcon() override { return ICON_TIMER; }
    char getIndicatorLetter() override { return '\0'; }

private:
    int64_t _lastToggleUs;
    bool _isRunning; // true = MANUAL running, false = OFF pause
    MaxFanState _runningState;
    MaxFanState _pausedState;
    CommandCallback _cb;
};

#endif
