#ifndef _MAX6675_H_
#include "pico/stdlib.h"
#include <stdio.h>
#define _MAX6675_H_

#define MIN_SAMPLE_TIME 300000UL // in uS
class MAX6675
{
public:
    MAX6675(uint8_t _sdo, uint8_t _sck, uint8_t _cs) : sdo(_sdo), sck(_sck), cs(_cs) {}
    void init();
    uint16_t sample();
    __force_inline bool isOpen() { return open; }
    __force_inline bool isExist() { return exist; }
    __force_inline uint16_t getLastResult() { return result; }
    __force_inline float toCelcius(uint16_t &sample) { return sample * 0.25; }

protected:
    uint8_t sdo, sck, cs;
    uint16_t result;
    bool open, exist;
    absolute_time_t lastSample;
};
#endif