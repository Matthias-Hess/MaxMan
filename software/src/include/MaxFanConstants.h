#include <stdlib.h>

#ifndef MAXFANCONSTANTS_H
#define MAXFANCONSTANTS_H

namespace MaxFan {

  static const uint8_t HEADER[] = { 
        0x5A, 0xA5, 0x80, 0x7F, 0x40, 0xBF, 0x20, 0xDF, 0x10, 0xCC 
    };

  static const uint8_t FOOTER[] = {
      0xFF, 0x23
    };



}

#define ENCODER_BUTTON 8
#define MODE_BUTTON 10
#define COVER_BUTTON 9

#endif // MAXFANCONSTANTS_H