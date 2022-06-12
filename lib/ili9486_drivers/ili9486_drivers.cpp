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
#include "hardware/adc.h"

/**
 *
 * @brief Construct a new ili9486 drivers object
 *
 * @param _pins_data a pointer to 8 size of uint8_t that correspond to D0~D7 pins of the panel (pointer content is copied by the class)
 * @param _pin_rst Reset pin of the panel
 * @param _pin_cs Chip Select pin of the panel
 * @param _pin_rs Command/Data pin of the panel
 * @param _pin_wr Write Data Signal of the panel
 * @param _pin_rd Read Data Signal of the panel
 * @param _pin_xp X positive pin of the resistive touchscreen
 * @param _pin_xm X negative pin of the resistive touchscreen
 * @param _pin_yp Y positive pin of the resistive touchscreen
 * @param _pin_ym Y negative pin of the resistive touchscreen
 */
ili9486_drivers::ili9486_drivers(uint8_t *_pins_data, uint8_t _pin_rst, uint8_t _pin_cs, uint8_t _pin_rs, uint8_t _pin_wr, uint8_t _pin_rd, uint8_t _pin_xp, uint8_t _pin_xm, uint8_t _pin_yp, uint8_t _pin_ym, uint8_t _xp_adc_channel, uint8_t _ym_adc_channel) : pin_rst(_pin_rst), pin_cs(_pin_cs), pin_rs(_pin_rs), pin_wr(_pin_wr), pin_rd(_pin_rd), pin_xp(_pin_xp), pin_xm(_pin_xm), pin_yp(_pin_yp), pin_ym(_pin_ym), xp_adc_channel(_xp_adc_channel), ym_adc_channel(_ym_adc_channel)
{
    memcpy(pins_data, _pins_data, sizeof(uint8_t) * 8);
    for (int i = 0; i < 8; i++)
    {
        gpio_init(pins_data[i]);
        gpio_set_dir(pins_data[i], GPIO_OUT);
    }

    adc_init();
    gpio_init(pin_xp);
    gpio_init(pin_xm);
    gpio_init(pin_yp);
    gpio_init(pin_ym);

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
    selectTFT();
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

    fillScreen(create565Color(255, 0, 0));
    deselectTFT();
}

void ili9486_drivers::sampleTouch(TouchCoordinate &tc)
{
    // The whole function typically takes 25uS at 250MHz overlocked sysclock (taken with time_us_64())
    deselectTFT(); // Make sure TFT is not selected before sampling the touchscreen
    uint16_t samples[2] = {0}; // X and Y is sampled twice
    gpio_set_dir(pin_yp, GPIO_OUT);
    gpio_set_dir(pin_ym, GPIO_OUT);
    gpio_set_dir(pin_xm, GPIO_IN);
    gpio_set_dir(pin_xp, GPIO_IN);
    adc_gpio_init(xp_adc_channel);

    gpio_put(pin_ym, 1);
    gpio_put(pin_yp, 0);

    adc_select_input(xp_adc_channel);
    samples[0] = adc_read();
    samples[1] = adc_read();
    tc.x = (samples[0] + samples[1]) / 2;

    gpio_set_dir(pin_xm, GPIO_OUT);
    gpio_set_dir(pin_xp, GPIO_OUT);
    gpio_set_dir(pin_yp, GPIO_IN);
    gpio_set_dir(pin_ym, GPIO_IN);
    adc_gpio_init(ym_adc_channel);

    gpio_put(pin_xm, 0);
    gpio_put(pin_xp, 1);

    adc_select_input(ym_adc_channel);
    samples[0] = adc_read();
    samples[1] = adc_read();
    tc.y = (samples[0] + samples[1]) / 2;

    // From AVR341:AVR341:Four and five-wire Touch Screen Controller
    gpio_set_dir(pin_yp, GPIO_OUT);
    gpio_set_dir(pin_xm, GPIO_OUT);
    gpio_set_dir(pin_ym, GPIO_IN);
    gpio_set_dir(pin_xp, GPIO_IN);
    adc_gpio_init(xp_adc_channel);
    adc_gpio_init(ym_adc_channel);

    gpio_put(pin_yp, 1);
    gpio_put(pin_xm, 0);

    adc_select_input(xp_adc_channel);
    uint16_t z1 = adc_read();
    adc_select_input(ym_adc_channel);
    uint16_t z2 = adc_read();

    // Equation 2.1 from AVR341 docs
    float rtouch;
    rtouch = touch_xPlateResistance * tc.x / 4096 * ((z2 / z1) - 1);
    tc.z = rtouch;

    gpio_set_dir(pin_yp, GPIO_OUT);
    gpio_set_dir(pin_xm, GPIO_OUT);
    gpio_set_dir(pin_ym, GPIO_OUT);
    gpio_set_dir(pin_xp, GPIO_OUT);
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

void ili9486_drivers::writeDataFast(uint8_t data)
{
    // putByte(data);
    gpio_put_masked(data_mask, data << 8);
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
    selectTFT();
    int len = 480 * 320;
    setAddressWindow(0, 0, 320, 480);
    writeCommand(CMD_MemoryWrite);
    gpio_put(pin_rs, 1);
    gpio_put(pin_rd, 1);
    while (len--)
    {
        writeDataFast(color >> 8);
        writeDataFast(color & 0xFF);
    }
    deselectTFT();
}

uint16_t ili9486_drivers::create565Color(uint8_t r, uint8_t g, uint8_t b)
{
    return ((r & 0x1F) << 11) | ((g & 0x3F) << 5) | (b & 0x1F);
}

void ili9486_drivers::putByte(uint8_t data)
{
    gpio_put_masked(data_mask, data << 8);
}
