/**
 * @file ili9486_drivers.h
 * @author Figo Arzaki Maulana (figoarzaki123@gmail.com)
 * @brief
 * @version 0.1
 * @date 2022-06-09
 *
 * @copyright Copyright (c) 2022
 *
 */

#ifndef _ILI9486_DRIVERS_H_
#define _ILI9486_DRIVERS_H_

#include "pico/stdlib.h"
#include "ili9486_commands.h"

/**
 * @brief
 * Struct that store width,height and rotation of the panel
 */
struct PanelConfig
{
    int16_t width, height;
    uint8_t rotation;
};

class ili9486_drivers
{
public:
    ili9486_drivers(uint8_t *_pins_data, uint8_t _pin_rst, uint8_t _pin_cs, uint8_t _pin_rs, uint8_t _pin_wr, uint8_t _pin_rd);
    void init();
    void setAddressWindow(int32_t x, int32_t y, int32_t w, int32_t h);
    void pushColors();
private:
    void dataDirectionRead();
    void dataDirectionWrite();
    void startWrite();
    void endWrite();
    void startRead();
    void endRead();
    void writeCommand(uint8_t cmd);
    void writeData(uint8_t data);
    void putByte(uint8_t data);
    uint8_t readData();
    /**
     * @brief
     * A 8-bit parallel bi-directional data bus for MCU system
     * Data setup and hold is 10ns
     */
    uint8_t pins_data[8];
    uint32_t data_mask = 0;

    /**
     * Low: the chip is selected and accessible
     * High: the chip is not selected and not accessible
     * Chip Select setup time (Write) 15 ns
     * Chip Select setup time (Read ID) 45 ns
     * Chip Select setup time (Read FM) 355 ns
     */
    uint8_t pin_cs;

    /**
     * Parallel interface (D/CX (RS)): The signal for command or
     * parameter select. LCD bus command / data selection signal.
     * Low: Command.
     * High: Parameter/Data.
     */
    uint8_t pin_rs;

    /**
    - Initializes the chip with a low input. Be sure to execute a
    power-on reset after supplying power.
    Apply signal longer than 10uS to reset the display
    */
    uint8_t pin_rst;

    /**
    - 8080 system (WRX): Serves as a write signal and writes data
    at the rising edge.
    Write cycle 50ns (max 20MHz)
    */
    uint8_t pin_wr;

    /**
    - 8080 system (RDX): Serves as a read signal and read data at
    the rising edge.
    */
    uint8_t pin_rd;

    PanelConfig panel_config;
};
#endif