#include "HC595.h"

void HC595::init()
{
    gpio_init(data);
    gpio_init(latch);
    gpio_init(clock);
    gpio_set_dir(data, GPIO_OUT);
    gpio_set_dir(latch, GPIO_OUT);
    gpio_set_dir(clock, GPIO_OUT);
    writeRegister(0x00);
}

void HC595::writeRegister(uint8_t _register)
{
    dataRegister = _register;

    uint8_t sspi_i;
    // Send 8 bits, with the MSB first.
    for (sspi_i = 0x80; sspi_i != 0x00; sspi_i >>= 1)
    {
        gpio_put(data, dataRegister & sspi_i);
        gpio_put(clock, 1);
        gpio_put(clock, 0);
    }
    gpio_put(latch, 0);
    gpio_put(latch, 1);
}