#ifndef MAXFANDISPLAY_H
#define MAXFANDISPLAY_H

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <MaxFanState.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

class MaxFanDisplay {
public:
    MaxFanDisplay(uint8_t sda, uint8_t scl);
    bool begin();
    
    // Die Hauptmethode: Aktualisiert den Bildschirm basierend auf dem Systemzustand
    void update(const MaxFanState& state, bool bleConnected, long encoderPos);

private:
    Adafruit_SSD1306 _display;
    uint8_t _sda, _scl;
    
    // Hilfsmethoden für Teilbereiche
    void drawHeader(bool bleConnected);
    void drawFanStats(const MaxFanState& state);
};

#endif