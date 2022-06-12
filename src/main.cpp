#include <stdio.h>
#include "pico/stdlib.h"
#include "ili9486_drivers.h"
#include "hardware/pll.h"
#include "hardware/clocks.h"
#include "hardware/structs/pll.h"
#include "hardware/structs/clocks.h"
#include "hardware/adc.h"
#include <tusb.h>

#define HIGH 1
#define LOW 0

static constexpr uint16_t xPlateResistance = 241; // Measured resistance between XP and XM
static constexpr uint16_t zThreshold = 20;

enum IO
{
    SD_CS = 5,
    SD_RX = 4,
    SD_SCK = 2,
    SD_TX = 7,

    TFT_D0 = 9,
    TFT_D1 = 8,
    TFT_D2 = 15,
    TFT_D3 = 14,
    TFT_D4 = 13,
    TFT_D5 = 12,
    TFT_D6 = 11,
    TFT_D7 = 10,

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
// ili9486_drivers tft(tft_dataPins, TFT_RST, TFT_CS, TFT_RS, TFT_WR, TFT_RD);

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

void readTouch(uint16_t &x, uint16_t &y, uint16_t &z)
{
    uint16_t samples[2] = {0}; // X and Y is sampled twice
    gpio_set_dir(TOUCH_YP, GPIO_OUT);
    gpio_set_dir(TOUCH_YM, GPIO_OUT);
    gpio_set_dir(TOUCH_XM, GPIO_IN);
    gpio_set_dir(TOUCH_XP, GPIO_IN);

    gpio_put(TOUCH_YM, 1);
    gpio_put(TOUCH_YP, 0);

    adc_select_input(TOUCH_XP_ADC_CHANNEL);
    samples[0] = adc_read();
    samples[1] = adc_read();
    x = (samples[0] + samples[1]) / 2;

    gpio_set_dir(TOUCH_XM, GPIO_OUT);
    gpio_set_dir(TOUCH_XP, GPIO_OUT);
    gpio_set_dir(TOUCH_YP, GPIO_IN);
    gpio_set_dir(TOUCH_YM, GPIO_IN);

    gpio_put(TOUCH_XM, 0);
    gpio_put(TOUCH_XP, 1);

    adc_select_input(TOUCH_YM_ADC_CHANNEL);
    samples[0] = adc_read();
    samples[1] = adc_read();
    y = (samples[0] + samples[1]) / 2;

    // From AVR341:AVR341:Four and five-wire Touch Screen Controller
    gpio_set_dir(TOUCH_YP, GPIO_OUT);
    gpio_set_dir(TOUCH_XM, GPIO_OUT);
    gpio_set_dir(TOUCH_YM, GPIO_IN);
    gpio_set_dir(TOUCH_XP, GPIO_IN);

    gpio_put(TOUCH_YP, 1);
    gpio_put(TOUCH_XM, 0);

    adc_select_input(TOUCH_XP_ADC_CHANNEL);
    uint16_t z1 = adc_read();
    adc_select_input(TOUCH_YM_ADC_CHANNEL);
    uint16_t z2 = adc_read();

    // Equation 2.1 from AVR341 docs
    float rtouch;
    rtouch = xPlateResistance * x / 4096 * ((z2 / z1) - 1);
    z = rtouch;
}

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
    // tft.init();

    adc_init();
    adc_gpio_init(TOUCH_XP_ADC_CHANNEL);
    adc_gpio_init(TOUCH_YM_ADC_CHANNEL);

    gpio_init(TOUCH_XP);
    gpio_init(TOUCH_XM);
    gpio_init(TOUCH_YM);
    gpio_init(TOUCH_YP);

    while (true)
    {
        uint16_t x = 0, y = 0, z = 0;
        readTouch(x, y, z);
        if (z > zThreshold)
            printf("x=%d y=%d z=%d\n", x, y, z);
        sleep_ms(100);
    }
    return 0;
}