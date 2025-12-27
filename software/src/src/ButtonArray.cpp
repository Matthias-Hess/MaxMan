#include "ButtonArray.h"

ButtonArray::ButtonArray(std::vector<uint8_t> pins) {
    for (uint8_t pin : pins) {
        // Initialisierung: Active-Low Logik
        _buttons.push_back({pin, false, false, 0});
    }
}

void ButtonArray::begin() {
    for (auto &btn : _buttons) {
        // Nutzt internen Pull-up; Button schaltet gegen GND
        pinMode(btn.pin, INPUT_PULLUP);
    }
    // Ticker startet alle 10ms
    _ticker.attach(0.01, timerISR, this);
}

void ButtonArray::updateAll() {
    for (auto &btn : _buttons) {
        // Bei Active-Low ist LOW (0) am Pin ein gedrückter Button
        bool isCurrentlyDown = (digitalRead(btn.pin) == LOW);

        if (isCurrentlyDown == btn.currentLevel) {
            btn.debounceCounter = 0;
        } else {
            btn.debounceCounter++;
            // Zustand muss 3 Ticks (30ms) stabil sein
            if (btn.debounceCounter >= 3) {
                btn.currentLevel = isCurrentlyDown;
                btn.debounceCounter = 0;
                if (btn.currentLevel) {
                    btn.wasPressedFlag = true;
                }
            }
        }
    }
}

void ButtonArray::timerISR(ButtonArray* instance) {
    instance->updateAll();
}

bool ButtonArray::wasPressed(uint8_t pin) {
    for (auto &btn : _buttons) {
        if (btn.pin == pin) {
            //noInterrupts();
            bool p = btn.wasPressedFlag;
            btn.wasPressedFlag = false;
            //interrupts();
            return p;
        }
    }
    return false;
}

bool ButtonArray::isPressed(uint8_t pin) {
    for (auto &btn : _buttons) {
        if (btn.pin == pin) return btn.currentLevel;
    }
    return false;
}