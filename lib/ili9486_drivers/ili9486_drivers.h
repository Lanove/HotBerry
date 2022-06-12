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

static constexpr uint16_t touch_xPlateResistance = 241; // Measured resistance between XP and XM
static constexpr uint16_t touch_zPressureMin = 20;
static constexpr uint16_t touch_zPressureMax = 1400;
static constexpr uint16_t touch_xCoordinateMax = 3400;
static constexpr uint16_t touch_xCoordinateMin = 400;
static constexpr uint16_t touch_yCoordinateMax = 3000;
static constexpr uint16_t touch_yCoordinateMin = 650;

struct TouchCoordinate{
    uint16_t x = 0,y = 0,z = 0;
};

/**
 * @brief
 * Struct that store width,height and rotation of the panel
 */
struct PanelConfig
{
    int16_t width, height;
    uint8_t rotation;
};

struct rgb565_t
{
    uint8_t r : 5;
    uint8_t g : 6;
    uint8_t b : 5;
};

class ili9486_drivers
{
public:
    ili9486_drivers(uint8_t *_pins_data, uint8_t _pin_rst, uint8_t _pin_cs, uint8_t _pin_rs, uint8_t _pin_wr, uint8_t _pin_rd, uint8_t _pin_xp, uint8_t _pin_xm, uint8_t _pin_yp, uint8_t _pin_ym, uint8_t _xp_adc_channel, uint8_t _ym_adc_channel);
    void init();
    void setAddressWindow(int32_t x, int32_t y, int32_t w, int32_t h);
    void setWindow(int32_t x0, int32_t y0, int32_t x1, int32_t y1);
    uint16_t create565Color(uint8_t r, uint8_t g, uint8_t b);
    void fillScreen(uint16_t color);
    void sampleTouch(TouchCoordinate &tc);
    __force_inline bool isTouchValid(TouchCoordinate &tc){ return tc.x < touch_xCoordinateMax && tc.x > touch_xCoordinateMin && tc.y < touch_yCoordinateMax && tc.y > touch_yCoordinateMin && tc.z < touch_zPressureMax && tc.z > touch_zPressureMin;}
private:
    __force_inline void selectTFT() { gpio_put(pin_cs, 0); }
    __force_inline void deselectTFT() { gpio_put(pin_cs, 1); }
    void writeCommand(uint8_t cmd);
    void writeData(uint8_t data);
    void putByte(uint8_t data);
    void writeDataFast(uint8_t data);
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

    /**
     * @brief
     * Resistive touch pins
     */
    uint8_t pin_xm, pin_xp, pin_yp, pin_ym;

    uint8_t xp_adc_channel, ym_adc_channel;

    PanelConfig panel_config;
};
#endif