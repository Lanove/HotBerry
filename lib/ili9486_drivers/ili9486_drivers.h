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

// Touch parameters
static constexpr uint16_t touch_xPlateResistance = 241; // Measured resistance between XP and XM
static constexpr uint16_t touch_zPressureMin = 20;
static constexpr uint16_t touch_zPressureMax = 1400;
static constexpr uint16_t touch_xADCMax = 3560;
static constexpr uint16_t touch_xADCMin = 370;
static constexpr uint16_t touch_yADCMax = 3270;
static constexpr uint16_t touch_yADCMin = 720;

// PIO Clock dividers
static constexpr uint32_t pio_clock_int_divider = 2;  // PIO runs at sysclk/2, 125MHz, (write cycle of 64ns)
static constexpr uint32_t pio_clock_frac_divider = 0; // PIO runs at sysclk/2, 125MHz, (write cycle of 64ns)

// Panel parameters
static constexpr uint16_t panel_width = 320;
static constexpr uint16_t panel_height = 480;

/**
 * @brief
 * Panel rotations enum
 */
enum Rotations
{
    PORTRAIT,
    LANDSCAPE,
    INVERTED_PORTRAIT,
    INVERTED_LANDSCAPE
};

/**
 * @brief Struct used to store touch coordinate and status
 */
struct TouchCoordinate
{
    uint16_t x = 0, y = 0, z = 0;
    bool touched;
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
    void dmaInit(void (*onComplete_cb)(void));
    /**
     * @brief
     * Check if DMA is still busy sending data, true if DMA is busy
     * @return bool
     */
    __force_inline bool dmaBusy() { return dma_channel_is_busy(dma_tx_channel); };
    /**
     * @brief
     * Wait for DMA to finish it's data transfer (blocking)
     */
    __force_inline void dmaWait() { dma_channel_wait_for_finish_blocking(dma_tx_channel); };
    /**
     * @brief
     * Clear DMA Interrupt Request (Must be called on onComplete_cb ISR from dmaInit())
     */
    __force_inline void dmaClearIRQ() { dma_hw->ints0 = 1u << dma_tx_channel; }
    /**
     * @brief
     * Get panel width (vary between 320 or 480 according to panel rotation)
     * @return uint16_t
     */
    __force_inline uint16_t width() { return _width; }
    /**
     * @brief
     * Get panel height (vary between 320 or 480 according to panel rotation)
     * @return uint16_t
     */
    __force_inline uint16_t height() { return _height; }
    /**
     * @brief Set the rotation of the panel
     * @param rt Rotations enum (choose between PORTRAIT,LANDSCAPE,INVERTED_PORTRAIT or INVERTED_LANDSCAPE)
     */
    __force_inline void setRotation(Rotations rt) { _rot = rt; }
    /**
     * @brief Activate/select TFT from receiving commands/data, will wait either DMA or PIO to finish it's operation before activate/select panel
     */
    __force_inline void selectTFT()
    {
        if (!dma_used)
            pio_waitForStall();
        else
            dmaWait();
        gpio_put(pin_cs, 0);
    }
    /**
     * @brief Deactivate/deselect TFT from receiving commands/data, will wait either DMA or PIO to finish it's operation before deactivate/deselect panel
     */
    __force_inline void deselectTFT()
    {
        if (!dma_used)
            pio_waitForStall();
        else
            dmaWait();
        gpio_put(pin_cs, 1);
    }

private:
    void pioInit(uint16_t clock_div, uint16_t fract_div);
    void pushBlock(uint16_t color, uint32_t len);
    void writeRotation();
    void writeData(uint8_t data);
    void writeCommand(uint8_t cmd);

    /**
     * @brief Wait until at least "count" number of FIFO is empty
     * @param count Number of FIFO
     */
    __force_inline void pio_waitForFreeFIFO(size_t count)
    {
        while (((tft_pio->flevel >> (pio_sm * 8)) & 0x000F) > (8 - count))
            ;
    }

    /**
     * @brief Wait for the PIO to stall (SM pull request finds no data in TX FIFO). This is used to detect when the SM is idle and hence ready for a jump instruction
     */
    __force_inline void pio_waitForStall()
    {
        tft_pio->fdebug = pull_stall_mask;
        while (!(tft_pio->fdebug & pull_stall_mask))
            ;
    }

    __force_inline void pio_enterCommandMode()
    {
        pio_waitForStall();
        tft_pio->sm[pio_sm].instr = pio_instr_clr_rs;
    }
    __force_inline void pio_enterDataMode()
    {
        pio_waitForStall();
        tft_pio->sm[pio_sm].instr = pio_instr_set_rs;
    }

    /// Panel rotation variable
    Rotations _rot = PORTRAIT;
    /// Panel width variable
    uint16_t _width = 0;
    /// Panel height variable
    uint16_t _height = 0;

    /// A 8-bit parallel bi-directional data bus for MCU system
    /// Data setup and hold is 10ns
    uint8_t pins_data[8];
    // GPIO data mask
    uint32_t data_mask = 0;

    /// Low: the LCD is selected and accessible
    /// High: the LCD is not selected and not accessible
    uint8_t pin_cs;

    /// The signal for command or parameter select. LCD bus command / data selection signal.
    /// Low: Command.
    /// High: Parameter/Data.
    uint8_t pin_rs;

    /// Initializes the chip with a low input. Be sure to execute a power-on reset after supplying power.
    /// Apply low signal longer than 10uS to reset the display
    uint8_t pin_rst;

    /// Serves as a write signal and writes data at the rising edge.
    uint8_t pin_wr;

    /// Serves as a read signal and read data at the rising edge.
    uint8_t pin_rd;

    /// Resistive touch pins
    uint8_t pin_xm, pin_xp, pin_yp, pin_ym;

    /// ADC channel used for touch ADC
    uint8_t xp_adc_channel, ym_adc_channel;

    // pioinit() will try both pio's to find a free SM
    PIO tft_pio = pio0;
    // pioinit() will claim free state machine available
    int8_t pio_sm = 0;
    // Updated later on pioinit() with the loading offset of the PIO program.
    uint32_t program_offset = 0;
    // PIO state machine stalled mask
    uint32_t pull_stall_mask = 0;
    // SM jump instructions to change SM behaviour
    uint32_t pio_instr_fill = 0; // Block fill instruction offset
    uint32_t pio_instr_addr = 0; // Set window address instruction offset
    uint32_t pio_instr_write8 = 0;

    // SM "set" instructions to control RS control signal
    uint32_t pio_instr_set_rs = 0;
    uint32_t pio_instr_clr_rs = 0;

    /// dmaInit() will claim free DMA channel available
    int32_t dma_tx_channel;
    /// Used to store used DMA channel configs
    dma_channel_config dma_tx_config;
    /// Flag if DMA is used or not, true if DMA is used
    bool dma_used;
};
#endif