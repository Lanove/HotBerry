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
#include "ili9486_drivers.h"

/**
 * @brief Construct a new ili9486 drivers::ili9486 drivers object
 *
 * @param _pins_data An array 8 size of uint8_t that correspond to D0~D7 pins of the panel, D0 to D7 must be sequentially incremental (ex GPIO0 to GPIO7)
 * @param _pin_rst Reset pin of the panel
 * @param _pin_cs Chip Select pin of the panel
 * @param _pin_rs Command/Data pin of the panel
 * @param _pin_wr Write Data Signal of the panel
 * @param _pin_rd Read Data Signal of the panel
 * @param _pin_xp X positive pin of the resistive touchscreen (Must be analog pin)
 * @param _pin_xm X negative pin of the resistive touchscreen
 * @param _pin_yp Y positive pin of the resistive touchscreen
 * @param _pin_ym Y negative pin of the resistive touchscreen (Must be analog pin)
 * @param _xp_adc_channel X positive ADC channel
 * @param _ym_adc_channel Y negative ADC channel
 */
ili9486_drivers::ili9486_drivers(uint8_t *_pins_data, uint8_t _pin_rst, uint8_t _pin_cs, uint8_t _pin_rs, uint8_t _pin_wr, uint8_t _pin_rd, uint8_t _pin_xp, uint8_t _pin_xm, uint8_t _pin_yp, uint8_t _pin_ym, uint8_t _xp_adc_channel, uint8_t _ym_adc_channel) : pin_rst(_pin_rst), pin_cs(_pin_cs), pin_rs(_pin_rs), pin_wr(_pin_wr), pin_rd(_pin_rd), pin_xp(_pin_xp), pin_xm(_pin_xm), pin_yp(_pin_yp), pin_ym(_pin_ym), xp_adc_channel(_xp_adc_channel), ym_adc_channel(_ym_adc_channel)
{
    // Store data pins array internally
    memcpy(pins_data, _pins_data, sizeof(uint8_t) * 8);

    // Initialize touch pins
    adc_init();
    gpio_init(pin_xp);
    gpio_init(pin_xm);
    gpio_init(pin_yp);
    gpio_init(pin_ym);

    // Initialize control pins (wr and rs handled by PIO init)
    gpio_init(pin_cs);
    gpio_init(pin_rst);
    gpio_init(pin_rd);
    gpio_set_dir(pin_cs, GPIO_OUT);
    gpio_set_dir(pin_rst, GPIO_OUT);
    gpio_set_dir(pin_rd, GPIO_OUT);
    gpio_put(pin_cs, 1);
    gpio_put(pin_rst, 1);
    gpio_put(pin_rd, 1);
}

/**
    Initialize panel, must be called before calling any draw functions
 */
void ili9486_drivers::init()
{
    // Hard reset the panel
    gpio_put(pin_rst, 1);
    sleep_ms(1);
    gpio_put(pin_rst, 0);
    sleep_us(15); // Reset pulse duration minimal 10uS
    gpio_put(pin_rst, 1);
    sleep_ms(120); // It is necessary to wait 5msec after releasing RESX before sending commands. Also Sleep Out command cannot be sent for 120msec.
    pioInit(pio_clock_int_divider, pio_clock_frac_divider);
    const uint8_t *initCommand_ptr = initCommands; // Load init command used
    selectTFT();
    for (;;) // For each command...
    {
        uint8_t cmd = *initCommand_ptr++; // Command instruction byte
        uint8_t num = *initCommand_ptr++; // Number of args to follow command
        if (cmd == 0xFF && num == 0xFF)   // If both cmd and num of arg is 0xFF, it's the end of initCommands
            break;
        writeCommand(cmd);
        uint_fast8_t ms = num & CMD_Delay; // Check if number of args contain CMD_Delay
        num &= ~CMD_Delay;                 // Mask out delay bit
        if (num)                           // If num is > 0 then
        {
            do // writeData for number of args following cmd
            {
                writeData(*initCommand_ptr++);
            } while (--num);
        }
        if (ms) // If CMD_Delay bit is set then delay before proceeding next command
        {
            ms = *initCommand_ptr++; // Following args after num is delay duration in ms (maximum 255ms or 1 byte)
            sleep_ms((ms == 255 ? 500 : ms));
        }
    }
    fillScreen(0); // Fill screen with black at init
    deselectTFT(); // Deselect TFT after PIO finishes transfer data from fillScreen()
}

/**
 * Private function to apply rotation setting from _rot to panel, uses regular GPIO function to write commands and data
 */
void ili9486_drivers::setRotation(Rotations rotation)
{
    _rot = rotation;
    selectTFT();
    writeCommand(CMD_MemoryAccessControl);

    switch (_rot)
    {
    case PORTRAIT: // Portrait
        writeData(MASK_BGR | MASK_MX);
        _width = panel_width;
        _height = panel_height;
        break;
    case LANDSCAPE: // Landscape (Portrait + 90)
        writeData(MASK_BGR | MASK_MV);
        _width = panel_height;
        _height = panel_width;
        break;
    case INVERTED_PORTRAIT: // Inverter portrait
        writeData(MASK_BGR | MASK_MY);
        _width = panel_width;
        _height = panel_height;
        break;
    case INVERTED_LANDSCAPE: // Inverted landscape
        writeData(MASK_BGR | MASK_MV | MASK_MX | MASK_MY);
        _width = panel_height;
        _height = panel_width;
        break;
    default: // Portrait
        writeData(MASK_BGR | MASK_MX);
        _width = panel_width;
        _height = panel_height;
        break;
    }
    deselectTFT();
}

void ili9486_drivers::pioInit(uint16_t clock_div, uint16_t fract_div)
{
    // Find a free SM on one of the PIO's
    tft_pio = pio0;
    pio_sm = pio_claim_unused_sm(tft_pio, false); // false means don't panic
    // Try pio1 if SM not found
    if (pio_sm < 0)
    {
        tft_pio = pio1;
        pio_sm = pio_claim_unused_sm(tft_pio, true); // panic this time if no SM is free
    }

    uint8_t bits = 8;

    // Load the PIO program
    program_offset = pio_add_program(tft_pio, &tft_io_program);

    // Associate pins with the PIO
    pio_gpio_init(tft_pio, pin_rs);
    pio_gpio_init(tft_pio, pin_wr);
    for (int i = 0; i < bits; i++)
        pio_gpio_init(tft_pio, pins_data[0] + i);

    // Configure the pins to be outputs
    pio_sm_set_consecutive_pindirs(tft_pio, pio_sm, pin_rs, 1, true);
    pio_sm_set_consecutive_pindirs(tft_pio, pio_sm, pin_wr, 1, true);
    pio_sm_set_consecutive_pindirs(tft_pio, pio_sm, pins_data[0], bits, true);

    // Configure the state machine
    pio_sm_config c = tft_io_program_get_default_config(program_offset);
    // Define the set pin
    sm_config_set_set_pins(&c, pin_rs, 1);
    // Define the single side-set pin
    sm_config_set_sideset_pins(&c, pin_wr);
    // Define the consecutive pins that are used for data output
    sm_config_set_out_pins(&c, pins_data[0], bits);
    // Set clock divider and fractional divider
    sm_config_set_clkdiv_int_frac(&c, clock_div, fract_div);
    // Make a single 8 words FIFO from the 4 words TX and RX FIFOs
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);
    // The OSR register shifts to the left, sm designed to send MS byte of a colour first
    sm_config_set_out_shift(&c, false, false, 0);
    // Now load the configuration
    pio_sm_init(tft_pio, pio_sm, program_offset + tft_io_offset_start_tx, &c);

    // Start the state machine.
    pio_sm_set_enabled(tft_pio, pio_sm, true);

    // Create the pull stall bit mask
    pull_stall_mask = 1u << (PIO_FDEBUG_TXSTALL_LSB + pio_sm);

    // Create the instructions for the jumps to send routines
    pio_instr_fill = pio_encode_jmp(program_offset + tft_io_offset_block_fill);
    pio_instr_addr = pio_encode_jmp(program_offset + tft_io_offset_set_addr_window);
    pio_instr_write8 = pio_encode_jmp(program_offset + tft_io_offset_start_8);

    // Create instruction to set and clear the RS signal
    pio_instr_set_rs = pio_encode_set((pio_src_dest)0, 1);
    pio_instr_clr_rs = pio_encode_set((pio_src_dest)0, 0);
}

void ili9486_drivers::dmaInit(void (*onComplete_cb)(void))
{
    dma_tx_channel = dma_claim_unused_channel(false);
    if (dma_tx_channel < 0)
        return;
    dma_tx_config = dma_channel_get_default_config(dma_tx_channel);
    channel_config_set_transfer_data_size(&dma_tx_config, DMA_SIZE_16);
    channel_config_set_dreq(&dma_tx_config, pio_get_dreq(tft_pio, pio_sm, true));
    dma_channel_set_irq0_enabled(dma_tx_channel, true);
    irq_set_exclusive_handler(DMA_IRQ_0, onComplete_cb);
    irq_set_enabled(DMA_IRQ_0, true);
    dma_used = true;
}

void ili9486_drivers::pushBlock(uint16_t color, uint32_t len)
{
    if (len)
    {
        pio_waitForStall();
        tft_pio->sm[pio_sm].instr = pio_instr_fill;

        tft_pio->txf[pio_sm] = color;
        tft_pio->txf[pio_sm] = --len; // Decrement first as PIO sends n+1
    }
}

void ili9486_drivers::setWindow(int32_t x0, int32_t y0, int32_t x1, int32_t y1)
{
    pio_waitForStall();
    tft_pio->sm[pio_sm].instr = pio_instr_addr;

    tft_pio->txf[pio_sm] = CMD_ColumnAddressSet;
    tft_pio->txf[pio_sm] = (x0 << 16) | x1;
    tft_pio->txf[pio_sm] = CMD_PageAddressSet;
    tft_pio->txf[pio_sm] = (y0 << 16) | y1;
    tft_pio->txf[pio_sm] = CMD_MemoryWrite;
}

void ili9486_drivers::setAddressWindow(int32_t x, int32_t y, int32_t w, int32_t h)
{
    int32_t xEnd = x + w - 1, yEnd = y + h - 1;
    setWindow(x, y, xEnd, yEnd);
}

void ili9486_drivers::fillScreen(uint16_t color)
{
    uint32_t len = _width * _height;
    setAddressWindow(0, 0, _width, _height);
    pushBlock(color, len);
}

void ili9486_drivers::pushColors(uint16_t *color, uint32_t len)
{
    const uint16_t *data = color;
    while (len > 4)
    {
        pio_waitForFreeFIFO(5);
        tft_pio->txf[pio_sm] = data[0];
        tft_pio->txf[pio_sm] = data[1];
        tft_pio->txf[pio_sm] = data[2];
        tft_pio->txf[pio_sm] = data[3];
        tft_pio->txf[pio_sm] = data[4];
        data += 5;
        len -= 5;
    }

    if (len)
    {
        pio_waitForFreeFIFO(4);
        while (len--)
            tft_pio->txf[pio_sm] = *data++;
    }
}

void ili9486_drivers::pushColorsDMA(uint16_t *colors, uint32_t len)
{
    if (!dma_used)
        return;
    channel_config_set_bswap(&dma_tx_config, false);
    dma_channel_configure(dma_tx_channel, &dma_tx_config, &tft_pio->txf[pio_sm], (uint16_t *)colors, len, true);
}

void ili9486_drivers::writeData(uint8_t data)
{
    pio_enterDataMode();
    tft_pio->sm[pio_sm].instr = pio_instr_write8;
    tft_pio->txf[pio_sm] = data;
}

void ili9486_drivers::writeCommand(uint8_t cmd)
{
    pio_enterCommandMode();
    tft_pio->sm[pio_sm].instr = pio_instr_write8;
    tft_pio->txf[pio_sm] = cmd;
}

void ili9486_drivers::sampleTouch(TouchCoordinate &tc)
{
    uint16_t x, y, z;
    // The whole function typically takes 25uS at 250MHz overlocked sysclock (taken with time_us_64())
    if (dmaBusy())
        return;
    deselectTFT();             // Make sure TFT is not selected before sampling the touchscreen
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
    x = (samples[0] + samples[1]) / 2; // needed to be stored, because pressure measurement needs X- and X+ touch measurement

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
    y = (samples[0] + samples[1]) / 2;

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
    rtouch = touch_xPlateResistance * x / 4096 * ((z2 / z1) - 1);
    z = rtouch;

    gpio_set_dir(pin_yp, GPIO_OUT);
    gpio_set_dir(pin_xm, GPIO_OUT);
    gpio_set_dir(pin_ym, GPIO_OUT);
    gpio_set_dir(pin_xp, GPIO_OUT);

    tc.touched = x < touch_xADCMax && x > touch_xADCMin &&
                 y < touch_yADCMax && y > touch_yADCMin &&
                 z < touch_zPressureMax && z > touch_zPressureMin;

    float x_tc = float(x - touch_xADCMin) / float(touch_xADCMax - touch_xADCMin) * (_rot == LANDSCAPE || _rot == INVERTED_LANDSCAPE ? _width : _height);
    float y_tc = float(y - touch_yADCMin) / float(touch_yADCMax - touch_yADCMin) * (_rot == LANDSCAPE || _rot == INVERTED_LANDSCAPE ? _height : _width);
    switch (_rot)
    {
    case LANDSCAPE:
        tc.x = x_tc;
        tc.y = _height - y_tc;
        break;
    case INVERTED_LANDSCAPE:
        tc.x = _width - x_tc;
        tc.y = y_tc;
        break;
    case PORTRAIT:
        tc.x = y_tc;
        tc.y = x_tc;
        break;
    case INVERTED_PORTRAIT:
        tc.x = _width - y_tc;
        tc.y = _height - x_tc;
        break;
    default:
        tc.x = y_tc;
        tc.y = x_tc;
        break;
    }
}

uint16_t ili9486_drivers::create565Color(uint8_t r, uint8_t g, uint8_t b)
{
    return ((r & 0x1F) << 11) | ((g & 0x3F) << 5) | (b & 0x1F);
}
