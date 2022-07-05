/**
 * @file MAX6675.h
 * @author Figo Arzaki Maulana (figoarzaki123@gmail.com)
 * @brief Simple library to interface to MAX6675 Thermocouple Type-K on RP2040
 * @version 0.1
 * @date 2022-07-05
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#ifndef _MAX6675_H_
#include "pico/stdlib.h"
#include <stdio.h>
#define _MAX6675_H_

#define CONVERSION_TIME 230000UL // in uS
class MAX6675
{
public:
    MAX6675(uint8_t _sdo, uint8_t _sck, uint8_t _cs) : sdo(_sdo), sck(_sck), cs(_cs) {}
    void init();
    uint16_t sample();
    __force_inline bool isOpen() { return open; }
    __force_inline bool isExist() { return exist; }
    // Get last sampled result without sampling the sensor
    __force_inline uint16_t getLastResult() { return result; }
    // Convert passed 12-bit adc to actual celcius
    __force_inline static float toCelcius(uint16_t sample) { return sample * 0.25; }

protected:
    uint8_t sdo, sck, cs;
    uint16_t result;
    bool open, exist;
    absolute_time_t lastSample;
};
#endif