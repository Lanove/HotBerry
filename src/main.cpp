#include <stdio.h>
#include "pico/stdlib.h"
#include "ili9486_drivers.h"
#include "hardware/pll.h"
#include "hardware/clocks.h"
#include "hardware/structs/pll.h"
#include "hardware/structs/clocks.h"
#include <tusb.h>

#define HIGH 1
#define LOW 0

enum IO
{
    TFT_D0 = 9,
    TFT_D1 = 8,
    TFT_D2 = 15,
    TFT_D3 = 14,
    TFT_D4 = 13,
    TFT_D5 = 12,
    TFT_D6 = 11,
    TFT_D7 = 10,

    TFT_CS = 26, // Also ADC0
    TFT_RS = 27, // ADC1
    TFT_WR = 28, // ADC2
    TFT_RD = 29, // ADC3
    TFT_RST = 25,

    THERM_DATA = 18,
    THERM_CS = 19,
    THERM_SCK = 20,
    SFT_DATA = 21,
    SFT_LATCH = 22,
    SFT_SCK = 23,
    I2C0_SDA = 0,
    I2C0_SCL = 1,
    SD_CS = 5,
    SD_DI = 4,
    SD_SCK = 2,
    SD_DO = 7,
    ENC_A = 6,
    ENC_B = 3,
    ENC_BTN = 24,
    UART0_TX = 16,
    UART0_RX = 17
};

uint8_t tft_dataPins[8] = {TFT_D0, TFT_D1, TFT_D2, TFT_D3, TFT_D4, TFT_D5, TFT_D6, TFT_D7};
ili9486_drivers tft(tft_dataPins, TFT_RST, TFT_CS, TFT_RS, TFT_WR, TFT_RD);

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
void measure_freqs(void) {
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

int main()
{
    stdio_init_all();
    while (!tud_cdc_connected()) { sleep_ms(100);  }
    printf("tud_cdc_connected()\n");

    if (!set_sys_clock_khz(250000, false))
      printf("system clock 250MHz failed\n");
    else
      printf("system clock now 250MHz\n");
    measure_freqs();
    tft.init();

    while (true)
    {
    }
    return 0;
}