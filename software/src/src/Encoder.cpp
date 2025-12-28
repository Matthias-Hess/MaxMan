#include "Encoder.h"
#include "soc/gpio_struct.h"
#include "hal/gpio_ll.h"

Encoder::Encoder(uint8_t a, uint8_t b)
: pinA(a), pinB(b), delta(0), position(0), lastState(0)
{
}

void Encoder::begin()
{
    pinMode(pinA, INPUT_PULLUP);
    pinMode(pinB, INPUT_PULLUP);

    uint8_t a = (GPIO.in.val >> pinA) & 1;
    uint8_t b = (GPIO.in.val >> pinB) & 1;
    lastState = (a << 1) | b;

    attachInterruptArg(digitalPinToInterrupt(pinA), isrHandler, this, CHANGE);
    attachInterruptArg(digitalPinToInterrupt(pinB), isrHandler, this, CHANGE);
}

void IRAM_ATTR Encoder::isrHandler(void* arg)
{
    static_cast<Encoder*>(arg)->handleISR();
}

void IRAM_ATTR Encoder::handleISR()
{


    uint8_t a = gpio_ll_get_level(&GPIO, (gpio_num_t)pinA);
    uint8_t b = gpio_ll_get_level(&GPIO, (gpio_num_t)pinB);
    uint8_t state = (a << 1) | b;

    if ((lastState == 0b00 && state == 0b01) ||
        (lastState == 0b01 && state == 0b11) ||
        (lastState == 0b11 && state == 0b10) ||
        (lastState == 0b10 && state == 0b00))
    {
        delta++;
        position++;
    }
    else if ((lastState == 0b00 && state == 0b10) ||
             (lastState == 0b10 && state == 0b11) ||
             (lastState == 0b11 && state == 0b01) ||
             (lastState == 0b01 && state == 0b00))
    {
        delta--;
        position--;
    }

    lastState = state;
}

int Encoder::getDelta()
{
    noInterrupts();
    int d = delta/4;
    delta = delta-4*d;
    interrupts();
    return d;
}

int Encoder::getPosition()
{
    noInterrupts();
    int p = position/4;
    interrupts();
    return p;
}

void Encoder::reset()
{
    noInterrupts();
    delta = 0;
    position = 0;
    interrupts();
}
