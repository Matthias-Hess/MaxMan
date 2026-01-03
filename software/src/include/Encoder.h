#pragma once

#include <Arduino.h>

class Encoder
{
public:
    Encoder(uint8_t pinA, uint8_t pinB);

    void begin();

    int getDelta();
    int getPosition();
    void reset();

    // Gibt die Zeit des letzten erkannten Inputs zurück (in Mikrosekunden seit Boot)
    int64_t getLastInputTime() const;

private:
    static void IRAM_ATTR isrHandler(void* arg);
    void IRAM_ATTR handleISR();

    uint8_t pinA;
    uint8_t pinB;

    volatile int delta;
    volatile int position;
    volatile uint8_t lastState;
    volatile int64_t _lastInputTime;
};
