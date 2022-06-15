#include <stdio.h>
#include "pico/stdlib.h"
#include "ili9486_drivers.h"
#include "hardware/pll.h"
#include "hardware/clocks.h"
#include "hardware/structs/pll.h"
#include "hardware/structs/clocks.h"
#include "hardware/structs/rosc.h"
#include "hardware/adc.h"
#include <tusb.h>
#include "lvgl.h"
#include "demos/lv_demos.h"

#define HIGH 1
#define LOW 0

enum IO
{
    SD_CS = 5,
    SD_RX = 4,
    SD_SCK = 2,
    SD_TX = 7,

    TFT_D0 = 8,
    TFT_D1 = 9,
    TFT_D2 = 10,
    TFT_D3 = 11,
    TFT_D4 = 12,
    TFT_D5 = 13,
    TFT_D6 = 14,
    TFT_D7 = 15,

    TFT_CS = 26, // Also ADC0
    TFT_RS = 27, // Also ADC1
    TFT_WR = 28, // Also ADC2
    TFT_RD = 29, // Also ADC3
    TFT_RST = 25,

    TOUCH_XP = TFT_RD, // ADC channel 3 (TFT_RD)
    TOUCH_YP = SD_TX,
    TOUCH_XM = SD_CS,
    TOUCH_YM = TFT_CS, // ADC channel 0 (TFT_CS)
    TOUCH_YM_ADC_CHANNEL = 0,
    TOUCH_XP_ADC_CHANNEL = 3,

    THERM_DATA = 18,
    THERM_CS = 19,
    THERM_SCK = 20,
    SFT_DATA = 21,
    SFT_LATCH = 22,
    SFT_SCK = 23,
    I2C0_SDA = 0,
    I2C0_SCL = 1,
    ENC_A = 6,
    ENC_B = 3,
    ENC_BTN = 24,
    UART0_TX = 16,
    UART0_RX = 17
};

uint8_t tft_dataPins[8] = {TFT_D0, TFT_D1, TFT_D2, TFT_D3, TFT_D4, TFT_D5, TFT_D6, TFT_D7};
ili9486_drivers *tft;

void shiftData8(uint8_t data)
{
    uint8_t sspi_i;
    // Send 8 bits, with the MSB first.
    for (sspi_i = 0x80; sspi_i != 0x00; sspi_i >>= 1)
    {
        gpio_put(21, data & sspi_i);
        gpio_put(23, 1);
        gpio_put(23, 0);
    }
}

void measure_freqs(void)
{
    uint f_pll_sys = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_PLL_SYS_CLKSRC_PRIMARY);
    uint f_pll_usb = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_PLL_USB_CLKSRC_PRIMARY);
    uint f_rosc = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_ROSC_CLKSRC);
    uint f_clk_sys = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS);
    uint f_clk_peri = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_PERI);
    uint f_clk_usb = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_USB);
    uint f_clk_adc = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_ADC);
    uint f_clk_rtc = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_RTC);

    printf("pll_sys  = %dkHz\n", f_pll_sys);
    printf("pll_usb  = %dkHz\n", f_pll_usb);
    printf("rosc     = %dkHz\n", f_rosc);
    printf("clk_sys  = %dkHz\n", f_clk_sys);
    printf("clk_peri = %dkHz\n", f_clk_peri);
    printf("clk_usb  = %dkHz\n", f_clk_usb);
    printf("clk_adc  = %dkHz\n", f_clk_adc);
    printf("clk_rtc  = %dkHz\n", f_clk_rtc);

    // Can't measure clk_ref / xosc as it is the ref
}

uint32_t rnd_whitened(void)
{
    int k, random = 0;
    int random_bit1, random_bit2;
    volatile uint32_t *rnd_reg = (uint32_t *)(ROSC_BASE + ROSC_RANDOMBIT_OFFSET);

    for (k = 0; k < 32; k++)
    {
        while (1)
        {
            random_bit1 = 0x00000001 & (*rnd_reg);
            random_bit2 = 0x00000001 & (*rnd_reg);
            if (random_bit1 != random_bit2)
                break;
        }

        random = random << 1;
        random = random + random_bit1;
    }
    return random;
}

void init_lvgl();
void lv_display_flush_cb(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p);
void lv_input_touch_cb(lv_indev_drv_t *indev_driver, lv_indev_data_t *data);
bool lv_tick_inc_cb(struct repeating_timer *t);
void lv_dma_onComplete_cb();

int main()
{
    uint32_t freq_mhz = 250;
    stdio_init_all();
    while (!tud_cdc_connected())
        sleep_ms(100);
    printf("tud_cdc_connected()\n");
    if (!set_sys_clock_khz(freq_mhz * 1000, false))
        printf("system clock %dMHz failed\n", freq_mhz);
    else
        printf("system clock now %dMHz\n", freq_mhz);
    measure_freqs();
    tft = new ili9486_drivers(tft_dataPins, TFT_RST, TFT_CS, TFT_RS, TFT_WR, TFT_RD, TOUCH_XP, TOUCH_XM, TOUCH_YP, TOUCH_YM, TOUCH_XP_ADC_CHANNEL, TOUCH_YM_ADC_CHANNEL);
    tft->setRotation(INVERTED_LANDSCAPE);
    tft->init();
    tft->dmaInit(lv_dma_onComplete_cb);
    init_lvgl();
    lv_demo_widgets();
    struct repeating_timer timer;
    add_repeating_timer_ms(5, lv_tick_inc_cb, NULL, &timer);
    while (true)
    {
        lv_task_handler();
        sleep_ms(1);
    }
    return 0;
}

static lv_disp_drv_t disp_drv;
void init_lvgl()
{
    lv_init();
    static constexpr size_t displayBufferSize = 480 * 80;
    /*A static or global variable to store the buffers*/
    static lv_disp_draw_buf_t disp_buf;

    /*Static or global buffer(s). The second buffer is optional*/
    static lv_color_t buf[displayBufferSize]; //
    // static lv_color_t buf2[displayBufferSize];

    /*Initialize `disp_buf` with the buffer(s). With only one buffer use NULL instead buf_2 */
    lv_disp_draw_buf_init(&disp_buf, buf, NULL, displayBufferSize);

    /*Initialize the display*/
    lv_disp_drv_init(&disp_drv);
    /*Change the following line to your display resolution*/
    disp_drv.hor_res = tft->width();
    disp_drv.ver_res = tft->height();
    disp_drv.flush_cb = lv_display_flush_cb;
    disp_drv.draw_buf = &disp_buf;
    lv_disp_drv_register(&disp_drv);

    /*Initialize the (dummy) input device driver*/
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = lv_input_touch_cb;
    lv_indev_drv_register(&indev_drv);
}

bool lv_tick_inc_cb(struct repeating_timer *t)
{
    lv_tick_inc(5);
    return true;
}

void lv_display_flush_cb(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    // if (!tft->dmaBusy())
    // {
    tft->selectTFT();
    tft->setAddressWindow(area->x1, area->y1, w, h);
    tft->pushColors((uint16_t *)&color_p->full, w * h);
    lv_disp_flush_ready(&disp_drv);
    // }
}

void lv_dma_onComplete_cb()
{
    lv_disp_flush_ready(&disp_drv);
    tft->dmaClearIRQ();
}

void lv_input_touch_cb(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
    TouchCoordinate tc;
    tft->sampleTouch(tc);
    if (!tc.touched)
        data->state = LV_INDEV_STATE_REL;
    else
    {
        data->state = LV_INDEV_STATE_PR;
        /*Set the coordinates*/
        data->point.x = tc.x;
        data->point.y = tc.y;
    }
}