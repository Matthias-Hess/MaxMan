#include "MaxFanDisplay.h"

MaxFanDisplay::MaxFanDisplay(uint8_t sda, uint8_t scl) 
: _display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1), _sda(sda), _scl(scl) {}

bool MaxFanDisplay::begin() {
    Wire.begin(_sda, _scl);
    if (!_display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        return false;
    }
    _display.clearDisplay();
    _display.display();
    return true;
}

void MaxFanDisplay::update(const MaxFanState& state, bool bleConnected, long encoderPos) {
    _display.clearDisplay();
    _display.setTextColor(SSD1306_WHITE);

    // 1. Header: BLE Status
    _display.setTextSize(1);
    _display.setCursor(0, 0);
    _display.print(bleConnected ? "BLE: ON" : "BLE: OFF");
    
    // 2. Hauptbereich: Lüfter-Daten
    _display.setTextSize(2);
    _display.setCursor(0, 20);
    _display.print("Speed: ");
    _display.print(state.GetSpeed()); // Annahme: Methode existiert
    
    _display.setCursor(0, 45);
    _display.setTextSize(1);
    _display.print("Temp: ");
    _display.print(state.GetTempByte());
    _display.print(" C");

    // 3. Encoder-Vorschau (unten rechts)
    _display.setCursor(90, 55);
    _display.print("[");
    _display.print(encoderPos);
    _display.print("]");

    _display.display();
}