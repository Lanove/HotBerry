#include <stdio.h>
#include "pico/stdlib.h"
#include "ili9486_drivers.h"

#define HIGH 1
#define LOW 0

typedef enum IO{
    /*
    A 8-bit parallel bi-directional data bus for MCU system
    Data setup and hold is 10ns
    */
    TFT_D0 = 9,
    TFT_D1 = 8,
    TFT_D2 = 15,
    TFT_D3 = 14,
    TFT_D4 = 13,
    TFT_D5 = 12,
    TFT_D6 = 11,
    TFT_D7 = 10,
    
    /*- A chip select signal.
    Low: the chip is selected and accessible
    High: the chip is not selected and not accessible
    Chip Select setup time (Write) 15 - ns -
    Chip Select setup time (Read ID) 45 - ns -
    Chip Select setup time (Read FM) 355 - ns -
    */
    TFT_CS = 26, // ADC0
    
    /*
    - Parallel interface (D/CX (RS)): The signal for command or
    parameter select. LCD bus command / data selection signal.
    Low: Command.
    High: Parameter/Data.
    */
    TFT_RS = 27, // ADC1
    
    /*
    - 8080 system (WRX): Serves as a write signal and writes data
    at the rising edge.
    Write cycle 50ns (max 20MHz)
    */
    TFT_WR = 28, // ADC2

    /*
    - 8080 system (RDX): Serves as a read signal and read data at
    the rising edge.
    */
    TFT_RD = 29, // ADC3

    /*- The external reset input.
    - Initializes the chip with a low input. Be sure to execute a
    power-on reset after supplying power.
    Apply signal longer than 10uS to reset the display
    */
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
    SD_SCK =  2,
    SD_DO = 7,
    ENC_A = 6,
    ENC_B = 3,
    ENC_BTN = 24,
    UART0_TX = 16,
    UART0_RX = 17
};

void init_tft_pins();

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
    bool led = false;

    const uint LED_PIN = 25;
    gpio_init(LED_PIN);
    gpio_init(21);
    gpio_init(22);
    gpio_init(23);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_set_dir(21, GPIO_OUT);
    gpio_set_dir(22, GPIO_OUT);
    gpio_set_dir(23, GPIO_OUT);
    shiftData8(0x00);
    gpio_put(22,0);
    gpio_put(22,1);

    while (true)
    {
        // uint8_t flash_id[8];
        // flash_get_unique_id(flash_id);
        // for (int i = 0; i < 8; i++)
        // {
        //     printf("%#X ", flash_id[i]);
        // }
        // printf("\n");
        shiftData8(0xFF);
        gpio_put(22,0);
        gpio_put(22,1);
        sleep_ms(1000);
        shiftData8(0x00);
        gpio_put(22,0);
        gpio_put(22,1);
        sleep_ms(1000);
    }
    return 0;
}

void init_tft_pins(){
    gpio_init(TFT_D0);
    gpio_init(TFT_D1);
    gpio_init(TFT_D2);
    gpio_init(TFT_D3);
    gpio_init(TFT_D4);
    gpio_init(TFT_D5);
    gpio_init(TFT_D6);
    gpio_init(TFT_D7);

    gpio_init(TFT_CS);
    gpio_init(TFT_RS);
    gpio_init(TFT_RST);
    gpio_init(TFT_WR);
    gpio_init(TFT_RD);

    gpio_set_dir(TFT_D0,GPIO_OUT);
    gpio_set_dir(TFT_D1,GPIO_OUT);
    gpio_set_dir(TFT_D2,GPIO_OUT);
    gpio_set_dir(TFT_D3,GPIO_OUT);
    gpio_set_dir(TFT_D4,GPIO_OUT);
    gpio_set_dir(TFT_D5,GPIO_OUT);
    gpio_set_dir(TFT_D6,GPIO_OUT);
    gpio_set_dir(TFT_D7,GPIO_OUT);

    gpio_set_dir(TFT_CS,GPIO_OUT);
    gpio_set_dir(TFT_RS,GPIO_OUT);
    gpio_set_dir(TFT_RST,GPIO_OUT);
    gpio_set_dir(TFT_WR,GPIO_OUT);
    gpio_set_dir(TFT_RD,GPIO_OUT);

    gpio_put(TFT_CS,HIGH);
    gpio_put(TFT_RST,HIGH);
}