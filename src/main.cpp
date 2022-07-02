#include "globals.h"
#include "lv_drivers.h"
#include "hardware/pll.h"
#include "hardware/clocks.h"
#include "hardware/structs/pll.h"
#include "hardware/structs/clocks.h"
#include "hardware/structs/rosc.h"
#include "hardware/adc.h"
#include <tusb.h>
#include "lvgl.h"
#include "demos/lv_demos.h"
#include "MAX6675.h"
#include "movingAvg.h"
#include "lv_app.h"

#define HIGH 1
#define LOW 0

MAX6675 *therm;
movingAvg temperature_raw(10);

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

int main()
{
    uint32_t freq_mhz = 250;
    stdio_init_all();
    // while (!tud_cdc_connected())
        // sleep_ms(100);
    printf("tud_cdc_connected()\n");
    if (!set_sys_clock_khz(freq_mhz * 1000, false))
        printf("system clock %dMHz failed\n", freq_mhz);
    else
        printf("system clock now %dMHz\n", freq_mhz);
    measure_freqs();
    temperature_raw.begin();
    therm = new MAX6675(THERM_DATA, THERM_SCK, THERM_CS);
    therm->init();
    init_display();

    // Read-only pointers
    static uint32_t bottomHeaterPV = 0;
    static uint32_t topHeaterPV = 0;
    static uint32_t secondsRunning = 0;

    // Read and write pointers
    static uint32_t bottomHeaterSV = 100;
    static uint32_t topHeaterSV = 150;
    static bool startedAuto;
    static bool startedManual;
    static float topHeaterP = 0.1;
    static float topHeaterI = 0.2;
    static float topHeaterD = 0.3;
    static float bottomHeaterP = 0.4;
    static float bottomHeaterI = 0.5;
    static float bottomHeaterD = 0.6;
    static uint8_t selectedProfile = 0;
    static Profile profileLists[10];

    {
        using namespace lv_app_pointers;

        pBottomHeaterPV = &bottomHeaterPV;
        pTopHeaterPV = &topHeaterPV;
        pSecondsRunning = &secondsRunning;

        pBottomHeaterSV = &bottomHeaterSV;
        pTopHeaterSV = &topHeaterSV;
        pStartedAuto = &startedAuto;
        pStartedManual = &startedManual;
        pTopHeaterPID[0] = &topHeaterP;
        pTopHeaterPID[1] = &topHeaterI;
        pTopHeaterPID[2] = &topHeaterD;
        pBottomHeaterPID[0] = &bottomHeaterP;
        pBottomHeaterPID[1] = &bottomHeaterI;
        pBottomHeaterPID[2] = &bottomHeaterD;
        pSelectedProfile = &selectedProfile;
        pProfileLists = &profileLists;

        (*pProfileLists)[0].dataPoint = 9;
        float ttemp[] = {30, 200, 200, 215, 215, 230, 240, 250, 0};
        int tsec[] = {0, 30, 90, 100, 130, 140, 170, 200, 220};
        (*pProfileLists)[0].startTopHeaterAt = 2;
        for (int i = 1; i < 10; i++)
            (*pProfileLists)[i].dataPoint = i;
        for (int i = 0; i < (*pProfileLists)[0].dataPoint; i++)
        {
            (*pProfileLists)[0].targetTemperature[i] = ttemp[i];
            (*pProfileLists)[0].targetSecond[i] = tsec[i];
        }
    }

    lv_app_entry();
    // lv_demo_widgets();
    while (true)
    {
        uint16_t therm_adc = therm->sample();
        if (therm_adc != 0xFFFF && therm_adc != 0xFFFE)
        {
            uint16_t avg = temperature_raw.reading(therm_adc);
            printf("raw %.2f avg %.2fC\n", therm->toCelcius(therm_adc), therm->toCelcius(avg));
        }
        lv_task_handler();


        static int counter = 0;
        counter++;
        if (counter >= 200)
        {
            counter = 0;
            topHeaterPV = rand() % (100 + 1);
            bottomHeaterPV = rand() % (100 + 1);
            if (startedAuto || startedManual)
            {
                secondsRunning++;
            }
        }
        sleep_ms(5);
    }
    return 0;
}