#include "MAX6675.h"

void MAX6675::init()
{
    gpio_init(sdo);
    gpio_init(sck);
    gpio_init(cs);
    gpio_set_dir(sdo, GPIO_IN);
    gpio_set_dir(sck, GPIO_OUT);
    gpio_set_dir(cs, GPIO_OUT);

    gpio_put(cs, 1);
    gpio_put(sck, 0);
}

uint16_t MAX6675::sample()
{
    // Sampling takes 4.61uS
    uint64_t diff = absolute_time_diff_us(lastSample, get_absolute_time());
    if (diff < MIN_SAMPLE_TIME)
        return 0xFFFE;
    uint16_t resp = 0;
    /*
        From Datasheet :
        CS Fall to SCK Rise minimal 100ns
        SCK Pulse high width : min 100ns
        SCK Pulse low width : min 100ns
        SCK max frequency : max 4.3MHz
        SCK Fall to output data valid : max 100ns
    */
    gpio_put(cs, 0);
    uint8_t i = 16;
    __asm__(
        "push {r7};"
        "movs r7, #5;"
        "1: sub r7, r7, #1;"
        "bne 1b;");
    while (i--) // Clock frequency 3.6MHz
    {
        gpio_put(sck, 1);
        __asm__(
            "movs r7, #7;"
            "1: sub r7, r7, #1;"
            "bne 1b;");
        resp |= (gpio_get(sdo) << i);
        gpio_put(sck, 0);
        __asm__(
            "movs r7, #7;"
            "1: sub r7, r7, #1;"
            "bne 1b;");
    }
    __asm__("pop {r7};");
    gpio_put(cs, 1);
    gpio_put(sck, 0);
    result = (resp & 0x7FF8) >> 3;
    open = resp & 0x04;
    // exist = resp & 0x02;
    if (open)
        return 0xFFFF;
    lastSample = get_absolute_time();
    return result;
}