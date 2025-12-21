#ifndef FANSTATECONVERTER_H
#define FANSTATECONVERTER_H

#include <Arduino.h>
#include "MaxReceiver.h"
#include "MaxFanBLE.h"

// Convert FanState (BLE/user-friendly format) to MaxFanCommand (IR protocol format)
MaxFanCommand fanStateToCommand(const FanState& state);

// Convert MaxFanCommand (IR protocol format) to FanState (BLE/user-friendly format)
FanState commandToFanState(const MaxFanCommand& cmd);

#endif // FANSTATECONVERTER_H

