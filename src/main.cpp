#include <stdio.h>
#include "pico/stdlib.h"
#include "ili9486_drivers.h"

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

int main()
{
    stdio_init_all();
    tft.init();

    while (true)
    {
    }
    return 0;
}