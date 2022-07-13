/**
 * @file AT24C16.h
 * @author Figo Arzaki Maulana (figoarzaki123@gmail.com)
 * @brief Simple driver to interface to AT24C16 EEPROM for RP2040 with FreeRTOS
 * AT24C16 is internally organized with 128 pages of 16 bytes each, it's total size is 16K Bits(2048 Bytes)
 * Note the following from the memory map.
    Each page requires 8-bits to to access the 256 locations/address
    Additional 3 bits are required to address 8 pages
    Hence 13 bits are required to address the entire 16K Memory.
  Page write can only be done 16-bytes at a time.
 * @version 0.1
 * @date 2022-07-03
 *
 * @copyright Copyright (c) 2022
 *
 */
#ifndef _AT24Cxx_H_
#include "pico/stdlib.h"
#include <hardware/i2c.h>
#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#define _AT24Cxx_H_

static constexpr uint8_t AT24C16_PageSize = 16;
static constexpr uint8_t AT24C16_i2cAddress = 0x50;

class AT24C16
{
  public:
    bool init(i2c_inst_t *i2c, uint sda, uint scl, uint32_t speed);
    void memWrite(uint16_t destAddress, const void *src, size_t len);
    void memRead(uint16_t srcAddress, void *dest, size_t len);
    void byteWrite(uint16_t destAddress, uint8_t byte);
    uint8_t byteRead(uint16_t srcAddress);
  private:
    i2c_inst_t *i2c_inst;
    void pageWrite(uint16_t address, const uint8_t *src, uint8_t len);
};
#endif