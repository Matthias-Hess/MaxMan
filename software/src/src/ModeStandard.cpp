#include "ModeStandard.h"

void ModeStandard::enter() {
    
    
    Serial.println(F("Entering Standard Mode"));
}

ModeAction ModeStandard::loop() {
    
    bool isConnected = _ble.isConnected();
  
    if (isConnected && !_wasConnected) {
        Serial.println("BLE client connected");
        _wasConnected = true;
    } else if (!isConnected && _wasConnected) {
        Serial.println("BLE client disconnected");
        _wasConnected = false;
        BLEDevice::startAdvertising(); 
    }

    if(isConnected){
        _ble.notifyStatus(_state.ToJson());
    }

    _remote.send(_state);
    _irReceiver.update(_state);
    _display.update(_state, isConnected, _testValue);

    int delta = _encoder.getDelta();
    
    if(delta != 0){
        switch (_state.GetMode()) {
            case MaxFanMode::OFF: 
                break;
            case MaxFanMode::MANUAL:
                _state.SetSpeed(_state.GetSpeed() - 10 * delta);
                break;
            case MaxFanMode::AUTO:
                _state.SetTempCelsius(_state.GetTempCelsius() - delta);
                break;
            default: 
                break;
        }
    }
    
    if (_buttons.hasEvent()) {
        KeyEvent event = _buttons.popEvent();

        if (event.IsSingle(ENCODER_BUTTON)) {
            Serial.println("EVENT: ENCODER_BUTTON");
            _state.SetAirFlow(_state.GetAirFlow() == MaxFanDirection::IN ? MaxFanDirection::OUT : MaxFanDirection::IN);
        }
        else if (event.IsSingle(COVER_BUTTON)) {
            Serial.println("EVENT: COVER_BUTTON");
            _state.SetCover(_state.GetCover() == CoverState::CLOSED ? CoverState::OPEN : CoverState::CLOSED);
        }
        else if (event.IsSingle(MODE_BUTTON)) {
            Serial.println("EVENT: MODE_BUTTON");
            switch (_state.GetMode()) {
                case MaxFanMode::OFF:
                    _state.SetMode(MaxFanMode::MANUAL);
                    break;
                case MaxFanMode::MANUAL:
                    _state.SetMode(MaxFanMode::AUTO);
                    break;
                case MaxFanMode::AUTO:
                    _state.SetMode(MaxFanMode::OFF);
                    break;
                default: break;
            }
        }

        else if (event.IsChord(MODE_BUTTON, COVER_BUTTON)) {
            Serial.println("EVENT: CHORD -> Switching to Config");
            return ModeAction::SWITCH_TO_CONFIG; 
        }
    }
    
    return ModeAction::NONE;
}