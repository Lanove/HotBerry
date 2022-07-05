#ifndef _GLOBALS_H
#define _GLOBALS_H
#include "pico/stdlib.h"
#include <stdio.h>

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
    SFT_CLOCK = 23,
    I2C0_SDA = 0,
    I2C0_SCL = 1,
    ENC_A = 6,
    ENC_B = 3,
    ENC_BTN = 24,
    UART0_TX = 16,
    UART0_RX = 17
};

enum SFTO
{
    AUX0 = 0,
    AUX1 = 1,
    ST_LED = 2,
    BUZZER = 3,
    SSR0 = 4,
    SSR1 = 5,
    LO1 = 6,
    LO2 = 7
};

static constexpr uint32_t cpu_freq_mhz = 250;

#endif