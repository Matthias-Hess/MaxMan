#include "TimerVentilationController.h"
#include "MaxFanConfig.h"
#include <string>
#include <esp_timer.h>
#include <Arduino.h>
#include "MaxFanState.h"

// Access the global state object from main
extern MaxFanState maxFanState;

TimerVentilationController::TimerVentilationController()
: _lastToggleUs(0), _isRunning(true), _cb(nullptr) {}

void TimerVentilationController::begin(const char* deviceName) {
    _lastToggleUs = esp_timer_get_time(); // microseconds
    _isRunning = true;

    // prepare running state
    {
        std::string af(GlobalConfig.timerAirflow);
        _runningState.SetMode(MaxFanMode::MANUAL);
        _runningState.SetCover(CoverState::OPEN);
        _runningState.SetAirFlow(toMaxFanDirection(af));
        _runningState.SetSpeed(GlobalConfig.timerPercent);
        _runningState.SetTempCelsius(maxFanState.GetTempCelsius());
    }
    // prepare paused (OFF) state
    {
        _pausedState.SetMode(MaxFanMode::OFF);
        _pausedState.SetCover(CoverState::CLOSED);
        _pausedState.SetTempCelsius(maxFanState.GetTempCelsius());
    }

    // Immediately apply the configured MANUAL state to global state
    maxFanState.SetBytes(_runningState.GetStateByte(), _runningState.GetSpeedByte(), _runningState.GetTempByte());
}

void TimerVentilationController::setCommandCallback(CommandCallback cb) {
    _cb = cb;
}

void TimerVentilationController::notifyStatus(const MaxFanState& state) {
    // no-op for this simple controller
}

bool TimerVentilationController::isConnected() {
    // This controller is local; treat as 'connected' for display purposes
    return true;
}

void TimerVentilationController::loop() {
    int64_t nowUs = esp_timer_get_time();

    // durations from config (seconds -> microseconds)
    int64_t runForUs = (int64_t)GlobalConfig.timerRunForSeconds * 1000000LL;
    int64_t pauseForUs = (int64_t)GlobalConfig.timerPauseForSeconds * 1000000LL;

    if (_isRunning) {
        // currently running MANUAL; wait until runFor elapsed
        if (nowUs - _lastToggleUs >= runForUs) {
            // copy paused state's core bytes into global state
            maxFanState.SetBytes(_pausedState.GetStateByte(), _pausedState.GetSpeedByte(), _pausedState.GetTempByte());
            _isRunning = false;
            _lastToggleUs = nowUs;
        }
    } else {
        // currently paused (OFF); wait until pause elapsed then start MANUAL again
        if (nowUs - _lastToggleUs >= pauseForUs) {
            // copy running state's core bytes into global state
            maxFanState.SetBytes(_runningState.GetStateByte(), _runningState.GetSpeedByte(), _runningState.GetTempByte());
            _isRunning = true;
            _lastToggleUs = nowUs;
        }
    }
}
