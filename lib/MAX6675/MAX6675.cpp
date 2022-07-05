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
    // This inline-assembly below basically create cycle accurate delay, total cycle delay is 3 times number moved to r7 
    // Cycle per instruction count taken from https://developer.arm.com/documentation/ddi0484/c/CHDCICDF
    __asm__(
        "push {r7};" // Temporarily store r7 register to stack, will return r7 back after soon, this take 1 cycle
        "movs r7, #5;" // Move 5 to r7, so this will take 15 cycle of delay, the mov instruction takes 1 cycle
        "1: sub r7, r7, #1;" // Substract r7 by 1, takes 1 cycle. also this is label "1"
        "bne 1b;"); // Jump back to 1 if r7 is != 0, takes 1 or 2 (?) cycle
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