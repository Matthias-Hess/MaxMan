#ifndef MAXFANDISPLAY_H
#define MAXFANDISPLAY_H

#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <MaxFanState.h>

class MaxFanDisplay {
public:
    MaxFanDisplay(uint8_t sda, uint8_t scl);
    bool begin();
    
    // Die Update-Methode zeichnet das komplette UI neu
    void update(const MaxFanState& state, bool bleConnected, long encoderPos);

private:
    // Wir nutzen den Hardware-I2C Treiber für SSD1306
    U8G2_SSD1306_128X64_NONAME_F_HW_I2C _u8g2;
    uint8_t _sda, _scl;
};

#endif