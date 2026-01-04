#ifndef NILCONTROLLER_H
#define NILCONTROLLER_H

#include "FanController.h"

class NilController : public FanController {
public:
    NilController() {}
    void begin(const char* deviceName = nullptr) override {}
    void setCommandCallback(CommandCallback cb) override { (void)cb; }
    void notifyStatus(const MaxFanState& state) override { (void)state; }
    void loop() override {}
    bool isConnected() override { return false; }
    Icon getIcon() override { return ICON_NONE; }
    char getIndicatorLetter() override { return '\0'; }
};

#endif
