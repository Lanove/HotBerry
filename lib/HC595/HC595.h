/**
 * @file HC595.h
 * @author your name (you@domain.com)
 * @brief Really simple library to interface only one HC595 shift register
 * @version 0.1
 * @date 2022-07-05
 *
 * @copyright Copyright (c) 2022
 *
 */
#ifndef _HC595_H_
#include "pico/stdlib.h"
#include <stdio.h>
#define _HC595_H_

class HC595
{
  public:
    HC595(uint8_t _data, uint8_t _latch, uint8_t _clock) : data(_data), latch(_latch), clock(_clock)
    {
    }
    void init();
    void writeRegister(uint8_t _register);
    __force_inline void writePin(uint8_t num, bool status)
    {
        writeRegister((dataRegister & ~(1 << num)) | (status << num));
    }
    __force_inline bool readPin(uint8_t num)
    {
        return ((1 << num) & dataRegister);
    }
    __force_inline uint8_t getDataRegister()
    {
        return dataRegister;
    }

  private:
    uint latch, clock, data;
    uint8_t dataRegister;
};
#endif