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
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "ili9486_commands.h"
#include "pio_8bit_parallel.pio.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/adc.h"
#include "hardware/irq.h"

// Wait for the PIO to stall (SM pull request finds no data in TX FIFO)
// This is used to detect when the SM is idle and hence ready for a jump instruction
#define WAIT_FOR_STALL                 \
    tft_pio->fdebug = pull_stall_mask; \
    while (!(tft_pio->fdebug & pull_stall_mask))

// Wait until at least "S" locations free
#define WAIT_FOR_FIFO_FREE(S)                                      \
    while (((tft_pio->flevel >> (pio_sm * 8)) & 0x000F) > (8 - S)) \
    {                                                              \
    }

// Wait until at least 5 locations free
#define WAIT_FOR_FIFO_5_FREE                             \
    while ((tft_pio->flevel) & (0x000c << (pio_sm * 8))) \
    {                                                    \
    }

// Wait until at least 1 location free
#define WAIT_FOR_FIFO_1_FREE                             \
    while ((tft_pio->flevel) & (0x0008 << (pio_sm * 8))) \
    {                                                    \
    }

// Wait for FIFO to empty (use before swapping to 8 bits)
#define WAIT_FOR_FIFO_EMPTY while (!(tft_pio->fstat & (1u << (PIO_FSTAT_TXEMPTY_LSB + pio_sm))))

// The write register of the TX FIFO.
#define TX_FIFO tft_pio->txf[pio_sm]

// PIO takes control of TFT_DC
// Must wait for data to flush through before changing DC line
#define RS_C        \
    WAIT_FOR_STALL; \
    tft_pio->sm[pio_sm].instr = pio_instr_clr_dc
// Flush has happened before this and mode changed back to 16 bit
#define RS_D tft_pio->sm[pio_sm].instr = pio_instr_set_dc

static constexpr uint16_t touch_xPlateResistance = 241; // Measured resistance between XP and XM
static constexpr uint16_t touch_zPressureMin = 20;
static constexpr uint16_t touch_zPressureMax = 1400;
static constexpr uint16_t touch_xCoordinateMax = 3560;
static constexpr uint16_t touch_xCoordinateMin = 370;
static constexpr uint16_t touch_yCoordinateMax = 3270;
static constexpr uint16_t touch_yCoordinateMin = 720;
// PIO runs at sysclk/2, 125MHz, (write cycle of 64ns)
static constexpr uint32_t pio_clock_int_divider = 2;
static constexpr uint32_t pio_clock_frac_divider = 0;
static constexpr uint16_t panel_width = 320;
static constexpr uint16_t panel_height = 480;

enum Rotations{
    PORTRAIT,
    LANDSCAPE,
    INVERTED_PORTRAIT,
    INVERTED_LANDSCAPE
};

struct TouchCoordinate
{
    uint16_t x = 0, y = 0, z = 0;
    bool touched;
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
    void pushColors(uint16_t *color, uint32_t len);
    void pushColorsDMA(uint16_t *colors, uint32_t len);
    void sampleTouch(TouchCoordinate &tc);
    __force_inline void setRotation(Rotations rt){_rot = rt;}
    __force_inline void selectTFT()
    {
        if(!dma_used) WAIT_FOR_STALL;
        gpio_put(pin_cs, 0);
    }
    __force_inline void deselectTFT()
    {
        if(!dma_used) WAIT_FOR_STALL;
        gpio_put(pin_cs, 1);
    }
    void dmaInit(void (*onComplete_cb)(void));
    __force_inline bool dmaBusy() { return dma_channel_is_busy(dma_tx_channel); };
    __force_inline void dmaWait() { dma_channel_wait_for_finish_blocking(dma_tx_channel); };
    __force_inline void dmaClearIRQ() { dma_hw->ints0 = 1u << dma_tx_channel; }
    __force_inline uint16_t width(){ return _width;}
    __force_inline uint16_t height(){ return _height;}
private:
    void pioInit(uint16_t clock_div, uint16_t fract_div);
    void pushBlock(uint16_t color, uint32_t len);
    void writeRotation();
    void writeData(uint8_t data);
    void writeCommand(uint8_t cmd);

    // Community RP2040 board package by Earle Philhower
    PIO tft_pio = pio0; // Code will try both pio's to find a free SM
    int8_t pio_sm = 0;  // pioinit will claim a free one
    // Updated later with the loading offset of the PIO program.
    uint32_t program_offset = 0;

    // SM stalled mask
    uint32_t pull_stall_mask = 0;

    // SM jump instructions to change SM behaviour
    uint32_t pio_instr_jmp8 = 0;
    uint32_t pio_instr_fill = 0;
    uint32_t pio_instr_addr = 0;

    // SM "set" instructions to control DC control signal
    uint32_t pio_instr_set_rs = 0;
    uint32_t pio_instr_clr_rs = 0;

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

    bool touch_swapxy = false;
    PanelConfig panel_config;

    int32_t dma_tx_channel;
    dma_channel_config dma_tx_config;
    bool dma_used;
    Rotations _rot = PORTRAIT;
    uint16_t _width = 0;
    uint16_t _height = 0;
};
#endif