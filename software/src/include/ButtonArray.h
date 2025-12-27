#ifndef BUTTONARRAY_H
#define BUTTONARRAY_H

#include <Arduino.h>
#include <Ticker.h>
#include <vector>

class ButtonArray {
public:
    struct ButtonState {
        uint8_t pin;
        volatile bool currentLevel;
        volatile bool wasPressedFlag;
        uint8_t debounceCounter;
    };

    // Nimmt eine Liste von Pins auf, z.B. {7, 8, 9}
    ButtonArray(std::vector<uint8_t> pins);
    void begin();
    
    bool wasPressed(uint8_t pin);
    bool isPressed(uint8_t pin);

private:
    std::vector<ButtonState> _buttons;
    Ticker _ticker;
    
    static void timerISR(ButtonArray* instance);
    void updateAll();
};

#endif