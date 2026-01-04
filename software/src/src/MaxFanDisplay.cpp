#include "MaxFanDisplay.h"
#include <cstring>

static const unsigned char image_manual_bits[] U8X8_PROGMEM = {0x00,0x00,0xfe,0x0f,0xfe,0x0f,0xfe,0x0f,0xfe,0x0f,0x00,0x00,0x00,0x00,0xfe,0x01,0xfe,0x01,0xfe,0x01,0xfe,0x01,0x00,0x00,0x00,0x00,0x3e,0x00,0x3e,0x00,0x3e,0x00,0x3e,0x00,0x00,0x00,0x00,0x00,0x0e,0x00,0x0e,0x00,0x0e,0x00,0x0e,0x00,0x00,0x00,0x00,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x06,0x00,0x00,0x00,0x00,0x00};
static const unsigned char image_auto_bits[] U8X8_PROGMEM = {0x00,0x00,0x78,0x07,0x78,0x00,0x48,0x01,0x48,0x00,0x48,0x07,0x48,0x00,0x48,0x01,0x48,0x00,0x48,0x07,0x48,0x00,0x48,0x01,0x48,0x00,0x78,0x07,0x78,0x00,0x78,0x01,0x78,0x00,0x78,0x07,0x78,0x00,0x78,0x01,0x78,0x00,0x78,0x07,0x78,0x00,0xfc,0x00,0x9e,0x01,0x9e,0x01,0xfe,0x01,0xfe,0x01,0xfc,0x00,0x78,0x00,0x00,0x00};


static const unsigned char image_open_bits[] U8X8_PROGMEM = {0x00,0xf0,0xff,0x00,0x00,0xf0,0xff,0x00,0x00,0x08,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x02,0x00,0x00,0xfc,0x03,0x00,0x3f,0x02,0x00,0x00,0x40,0x81,0x39,0x2f,0x82,0x41,0x4a,0x21,0x82,0x41,0x4a,0x67,0x82,0x41,0x3a,0xa1,0x82,0x41,0x0a,0x21,0x83,0x41,0x0a,0x21,0x82,0x81,0x09,0x2f,0x82,0x02,0x00,0x00,0x40,0xfc,0xff,0xff,0x3f};
static const unsigned char image_closed_bits[] U8X8_PROGMEM = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xfe,0xff,0x00,0x00,0xfe,0xff,0x00,0xfc,0x01,0x00,0x3f,0x02,0x00,0x00,0x40,0x99,0x30,0xe6,0x9d,0xa5,0x48,0x29,0xa4,0x85,0x48,0xe1,0xa4,0x85,0x48,0x26,0xa4,0x85,0x48,0x28,0xa4,0xa5,0x48,0x29,0xa4,0x99,0x33,0xe6,0x9d,0x02,0x00,0x00,0x40,0xfc,0xff,0xff,0x3f};

static const unsigned char image_in_bits[] U8X8_PROGMEM = {0x00,0x00,0xfe,0x0f,0x00,0x00,0xfc,0x07,0xfc,0xff,0xf8,0x03,0x02,0x00,0xf0,0x01,0x29,0x02,0xe0,0x20,0x29,0x02,0x40,0x20,0x69,0x02,0x00,0x20,0xa9,0x02,0x00,0x20,0x29,0x03,0x00,0x20,0x29,0x02,0x00,0x20,0x29,0x02,0x00,0x20,0x02,0x00,0x00,0x10,0xfc,0xff,0xff,0x0f};
static const unsigned char image_out_bits[] U8X8_PROGMEM = {0x00,0x00,0x40,0x00,0x00,0x00,0xe0,0x00,0xfc,0xff,0xf3,0x09,0x02,0x00,0xf8,0x13,0x99,0xf4,0xfd,0x27,0xa5,0x44,0xfe,0x2f,0xa5,0x44,0x00,0x20,0xa5,0x44,0x00,0x20,0xa5,0x44,0x00,0x20,0xa5,0x44,0x00,0x20,0x19,0x43,0x00,0x20,0x02,0x00,0x00,0x10,0xfc,0xff,0xff,0x0f};

static const unsigned char image_BTConnected_bits[] U8X8_PROGMEM = {0x10,0x31,0x52,0x94,0x58,0x38,0x54,0x92,0x51,0x30,0x10};
static const unsigned char image_mqtt_bits[] U8X8_PROGMEM = {0x4f,0x10,0x27,0x48,0x53,0x57,0x57};

MaxFanDisplay::MaxFanDisplay(uint8_t sda, uint8_t scl) 
: _u8g2(U8G2_R0, U8X8_PIN_NONE), _sda(sda), _scl(scl) {}
bool MaxFanDisplay::begin() {
    Wire.begin(_sda, _scl);
    _u8g2.begin();
    _u8g2.clearBuffer();
    _u8g2.sendBuffer();
    return true;
}




void drawOff(const MaxFanState& state, U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2){
    u8g2.setFont(u8g2_font_t0_11_tr);
    u8g2.setDrawColor(1);
    u8g2.setFont(u8g2_font_helvB18_tf);
    u8g2.drawStr(35, 42, "OFF");
}

void drawManual(const MaxFanState& state, U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2){
    u8g2.drawXBMP(5, 18, 13, 31, image_manual_bits);
    u8g2.setDrawColor(1);
    u8g2.setFont(u8g2_font_helvB18_tf);
    String speedStr;
    speedStr = String(state.GetSpeed()) + " %";
    u8g2.drawStr(35, 42, speedStr.c_str());
}

void drawAuto(const MaxFanState& state, U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2){
    u8g2.drawXBMP(5, 17, 13, 31, image_auto_bits);
    u8g2.setDrawColor(1);
    u8g2.setFont(u8g2_font_helvB18_tf);
    String tempStr;
    tempStr = String(state.GetTempCelsius()) + " " "\xC2\xB0" "C"; 
    u8g2.drawUTF8(35, 42, tempStr.c_str());
}

void MaxFanDisplay::showError(MaxError error) {
    _activeError = error;
    _errorStartTime = esp_timer_get_time();
}



void MaxFanDisplay::update(const MaxFanState& state, RemoteAccess::Icon icon, bool isConnected, char indicator, long encoderPos) {
    _u8g2.clearBuffer();
    _u8g2.setFontMode(1);
    _u8g2.setBitmapMode(1);

     MaxFanMode mode = state.GetMode();

     // MenuBar
     _u8g2.drawBox(0, 0, 127, 14);
     
   
     _u8g2.setDrawColor(2);

     // OffRect
     if(mode == MaxFanMode::OFF){
        _u8g2.drawBox(1, 1, 23, 12);
     } else {
        _u8g2.drawFrame(1, 1, 23, 12);
     }
     
     // OffText
     _u8g2.setFont(u8g2_font_t0_11_tr);
     _u8g2.drawStr(3, 11, "OFF");
 
     
     // ManuellRect
     if(mode == MaxFanMode::MANUAL){
        _u8g2.drawBox(38, 1, 45, 12);
     } else {
        _u8g2.drawFrame(38, 1, 45, 12);
     }
     
     // ManuellText
     _u8g2.setFont(u8g2_font_t0_12_tr);
     _u8g2.drawStr(40, 11, "MANUELL");
     
     
     // AutoRect
    if(mode == MaxFanMode::AUTO){
        _u8g2.drawBox(97, 1, 28, 12);
     } else {
        _u8g2.drawFrame(97, 1, 28, 12);
     }

     // AutoText
     _u8g2.setFont(u8g2_font_t0_11_tr);
     _u8g2.drawStr(99, 11, "AUTO");
     
     
     
     
     

    switch (state.GetMode())
    {
      case MaxFanMode::OFF:
        drawOff(state, _u8g2);
        break;

      case MaxFanMode::MANUAL:
        drawManual(state, _u8g2);
        break;

      case MaxFanMode::AUTO:
        drawAuto(state, _u8g2);
        break;
        
      default:
        break;
    }

    // Connection icon handling: `icon` selects icon, `isConnected` controls highlighted (filled box + XOR draw)
    if (icon == RemoteAccess::ICON_MQTT) {
        const int iconW = 7;
        const int iconH = 7;
        const int iconX = 116;
        const int iconY = 20;
        // If an indicator letter is present, render it left of the icon and do not draw the filled rectangle.
        if (indicator != '\0') {
            _u8g2.setFont(u8g2_font_t0_11_tr);
            _u8g2.setDrawColor(1);
            // draw letter slightly left and centered vertically to the icon
            _u8g2.drawStr(iconX - 10, iconY + (iconH / 2) + 4, String(indicator).c_str());
            _u8g2.setDrawColor(1);
            _u8g2.drawXBMP(iconX, iconY, iconW, iconH, image_mqtt_bits);
        } else if (isConnected) {
            _u8g2.setDrawColor(1);
            _u8g2.drawBox(iconX - 2, iconY - 2, iconW + 4, iconH + 4);
            _u8g2.setDrawColor(2);
            _u8g2.drawXBMP(iconX, iconY, iconW, iconH, image_mqtt_bits);
        } else {
            _u8g2.setDrawColor(1);
            _u8g2.drawXBMP(iconX, iconY, iconW, iconH, image_mqtt_bits);
        }
    } else if (icon == RemoteAccess::ICON_BLE) {
        const int iconW = 8;
        const int iconH = 11;
        const int iconX = 116;
        const int iconY = 18;
        if (indicator != '\0') {
            _u8g2.setFont(u8g2_font_t0_11_tr);
            _u8g2.setDrawColor(1);
            _u8g2.drawStr(iconX - 10, iconY + (iconH / 2) + 4, String(indicator).c_str());
            _u8g2.setDrawColor(1);
            _u8g2.drawXBMP(iconX, iconY, iconW, iconH, image_BTConnected_bits);
        } else if (isConnected) {
            _u8g2.setDrawColor(1);
            _u8g2.drawBox(iconX - 2, iconY - 2, iconW + 4, iconH + 4);
            _u8g2.setDrawColor(2);
            _u8g2.drawXBMP(iconX, iconY, iconW, iconH, image_BTConnected_bits);
        } else {
            _u8g2.setDrawColor(1);
            _u8g2.drawXBMP(iconX, iconY, iconW, iconH, image_BTConnected_bits);
        }
    }

    
    if (state.GetCover() == CoverState::OPEN) {
         _u8g2.drawXBMP(90, 48, 32, 16, image_open_bits);
    } else {
        _u8g2.drawXBMP(90, 48, 32, 16, image_closed_bits);
    }

   if (state.GetAirFlow() == MaxFanDirection::IN) {
        _u8g2.drawXBMP(4, 51, 30, 13, image_in_bits);
    } else {
        _u8g2.drawXBMP(4, 51, 30, 13, image_out_bits);
    }
        
   
    if (_activeError != MaxError::NONE) {
        // Zeit prüfen
        if (esp_timer_get_time() - _errorStartTime > _errorDuration) {
            _activeError = MaxError::NONE; // Fehlerzeit abgelaufen
        } else {
            _u8g2.clearBuffer();
            _u8g2.setFontMode(1);
            _u8g2.setBitmapMode(1);
            
            // outerBox
            _u8g2.drawBox(4, 5, 119, 45);

            // innerBox
            _u8g2.setDrawColor(2);
            _u8g2.drawBox(5, 19, 117, 30);

            // Caption
            _u8g2.setFont(u8g2_font_profont11_tr);
            _u8g2.drawStr(10, 16, getMaxErrorCaption(_activeError));

            // message
            _u8g2.setFont(u8g2_font_profont12_tr);
            _u8g2.drawStr(13, 37, getMaxErrorText(_activeError));
        }
    }
    
    
    _u8g2.sendBuffer();
}
