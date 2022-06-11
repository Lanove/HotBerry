/**
 * @file ili9486_drivers.cpp
 * @author Figo Arzaki Maulana (figoarzaki123@gmail.com)
 * @brief
 * @version 0.1
 * @date 2022-06-09
 *
 * @copyright Copyright (c) 2022
 *
 */
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "ili9486_drivers.h"

/**
 * @brief Construct a new ili9486 drivers object
 *
 * @param _pins_data a pointer to 8 size of uint8_t that correspond to D0~D7 pins of the panel (pointer content is copied by the class)
 * @param _pin_rst Reset pin of the panel
 * @param _pin_cs Chip Select pin of the panel
 * @param _pin_rs Command/Data pin of the panel
 * @param _pin_wr Write Data Signal of the panel
 * @param _pin_rd Read Data Signal of the panel
 */
ili9486_drivers::ili9486_drivers(uint8_t *_pins_data, uint8_t _pin_rst, uint8_t _pin_cs, uint8_t _pin_rs, uint8_t _pin_wr, uint8_t _pin_rd) : pin_rst(_pin_rst), pin_cs(_pin_cs), pin_rs(_pin_rs), pin_wr(_pin_wr), pin_rd(_pin_rd)
{
    memcpy(pins_data, _pins_data, sizeof(uint8_t) * 8);
    for (int i = 0; i < 8; i++)
    {
        gpio_init(pins_data[i]);
        gpio_set_dir(pins_data[i], GPIO_OUT);
    }
    
    gpio_init(pin_cs);
    gpio_init(pin_rs);
    gpio_init(pin_rst);
    gpio_init(pin_wr);
    gpio_init(pin_rd);

    gpio_set_dir(pin_cs, GPIO_OUT);
    gpio_set_dir(pin_rs, GPIO_OUT);
    gpio_set_dir(pin_rst, GPIO_OUT);
    gpio_set_dir(pin_wr, GPIO_OUT);
    gpio_set_dir(pin_rd, GPIO_OUT);

    gpio_put(pin_cs, 1);
    gpio_put(pin_rst, 1);
    gpio_put(pin_wr, 1);
    gpio_put(pin_rd, 1);
    for (int i = 0; i < 8; i++)
        data_mask |= 1 << pins_data[i];
}

void ili9486_drivers::init()
{
    gpio_put(pin_rst, 1);
    sleep_ms(1);
    gpio_put(pin_rst, 0);
    sleep_us(15); // Reset pulse duration minimal 10uS
    gpio_put(pin_rst, 1);
    sleep_ms(120); // It is necessary to wait 5msec after releasing RESX before sending commands. Also Sleep Out command cannot be sent for 120msec.
    const uint8_t *initCommand_ptr = initCommands_bodmer;
    startWrite();
    for (;;)
    { // For each command...
        uint8_t cmd = *initCommand_ptr++;
        uint8_t num = *initCommand_ptr++; // Number of args to follow
        if (cmd == 0xFF && num == 0xFF)
            break;
        writeCommand(cmd);                 // Read, issue command
        uint_fast8_t ms = num & CMD_Delay; // If hibit set, delay follows args
        num &= ~CMD_Delay;                 // Mask out delay bit
        if (num)
        {
            do
            {                                  // For each argument...
                writeData(*initCommand_ptr++); // Read, issue argument
            } while (--num);
        }
        if (ms)
        {
            ms = *initCommand_ptr++; // Read post-command delay time (ms)
            sleep_ms((ms == 255 ? 500 : ms));
        }
    }

    setAddressWindow(0, 0, 320, 480);
    fillScreen(create565Color(255,0,0));
    endWrite();
}

void ili9486_drivers::startWrite()
{
    gpio_put(pin_cs, 0);
}

void ili9486_drivers::endWrite()
{
    gpio_put(pin_cs, 1);
}

void ili9486_drivers::writeCommand(uint8_t cmd)
{
    gpio_put(pin_rs, 0);
    gpio_put(pin_rd, 1);

    putByte(cmd);
    gpio_put(pin_wr, 0);
    gpio_put(pin_wr, 1);
}

void ili9486_drivers::writeData(uint8_t data)
{
    gpio_put(pin_rs, 1);
    gpio_put(pin_rd, 1);

    putByte(data);
    gpio_put(pin_wr, 0);
    gpio_put(pin_wr, 1);
}

void ili9486_drivers::setWindow(int32_t x0, int32_t y0, int32_t x1, int32_t y1)
{
    writeCommand(CMD_ColumnAddressSet);
    writeData(x0 >> 8);
    writeData(x0 & 0xFF);
    writeData(x1 >> 8);
    writeData(x1 & 0xFF);

    writeCommand(CMD_PageAddressSet);
    writeData(y0 >> 8);
    writeData(y0 & 0xFF);
    writeData(y1 >> 8);
    writeData(y1 & 0xFF);
}

void ili9486_drivers::setAddressWindow(int32_t x, int32_t y, int32_t w, int32_t h)
{
    int16_t xEnd = x + w - 1, yEnd = y + h - 1;
    setWindow(x, y, xEnd, yEnd);
}

void ili9486_drivers::fillScreen(uint16_t color)
{
    int len = 480*320;
    writeCommand(CMD_MemoryWrite);
    while (len--)
    {
        writeData(color >> 8);
        writeData(color & 0xFF);
    }
}

uint16_t ili9486_drivers::create565Color(uint8_t r, uint8_t g, uint8_t b)
{
    return ((r & 0x1F) << 11) | ((g & 0x3F) << 5) | (b & 0x1F);
}

void ili9486_drivers::putByte(uint8_t data)
{
    uint32_t output_mask = 0;
    for (int i = 0; i < 8; i++)
    {
        output_mask |= (((data) >> (i)) & 0x01) << pins_data[i];
    }
    gpio_put_masked(data_mask, output_mask);
}
