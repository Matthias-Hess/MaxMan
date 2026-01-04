#include "ModeStandard.h"
#include "MaxFanConfig.h"
#include "MaxFanConstants.h"
#include <esp_timer.h>

void ModeStandard::enter() {
    
    
    Serial.println(F("Entering Standard Mode"));
}

ModeAction ModeStandard::loop() {
    
    bool isConnected = _remoteAccess.isConnected();

    if(isConnected){
        _remoteAccess.notifyStatus(_state);
    }

    _remote.send(_state);
    _irReceiver.update(_state);
        _display.update(_state, _remoteAccess.getIcon(), isConnected, _remoteAccess.getIndicatorLetter(), _testValue);

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
    
    // Check for timeout to switch to ScreenDark mode
    // Only check if timeout is enabled (displayTimeoutSeconds > 0)
    if (GlobalConfig.displayTimeoutSeconds > 0) {
        int64_t now = esp_timer_get_time();
        int64_t encoderLastInput = _encoder.getLastInputTime();
        int64_t buttonsLastInput = _buttons.getLastInputTime();
        
        // Get the most recent input time
        int64_t lastInputTime = (encoderLastInput > buttonsLastInput) ? encoderLastInput : buttonsLastInput;
        
        // Calculate timeout in microseconds
        int64_t timeoutUs = GlobalConfig.displayTimeoutSeconds * 1000000LL;
        
        // If no input for the timeout period, switch to screen dark mode
        if (now - lastInputTime > timeoutUs) {
            Serial.println(F("Standard Mode: Timeout reached, switching to Screen Dark Mode"));
            return ModeAction::SWITCH_TO_SCREEN_DARK;
        }
    }
    
    return ModeAction::NONE;
}