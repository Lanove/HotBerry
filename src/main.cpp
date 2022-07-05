#include "MAX6675.h"
#include "globals.h"
#include "hardware/adc.h"
#include "hardware/clocks.h"
#include "hardware/pll.h"
#include "hardware/structs/clocks.h"
#include "hardware/structs/pll.h"
#include "hardware/structs/rosc.h"
#include "lv_app.h"
#include "lv_drivers.h"
#include "lvgl.h"
#include "movingAvg.h"
#include <tusb.h>

#define HIGH 1
#define LOW 0

MAX6675 top_max6675(THERM_DATA, THERM_SCK, THERM_CS);
MAX6675 bottom_max6675(THERM_DATA, THERM_SCK, UART0_RX);
movingAvg adc_topHeater(10);
movingAvg adc_bottomHeater(10);

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
    while (!tud_cdc_connected())
        sleep_ms(100);
    if (!set_sys_clock_khz(cpu_freq_mhz * 1000, false))
        printf("set system clock to %dMHz failed\n", cpu_freq_mhz);
    else
        printf("system clock is now %dMHz\n", cpu_freq_mhz);
    top_max6675.init();
    bottom_max6675.init();
    adc_topHeater.begin();
    adc_bottomHeater.begin();
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
    static float topHeaterPID[3];
    static float bottomHeaterPID[3];
    static uint16_t selectedProfile = 0;
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
        pTopHeaterPID = &topHeaterPID;
        pBottomHeaterPID = &bottomHeaterPID;
        pSelectedProfile = &selectedProfile;
        pProfileLists = &profileLists;
    }

    lv_app_entry();
    // lv_demo_widgets();
    while (true)
    {
        uint16_t top_adc = top_max6675.sample();
        uint16_t bottom_adc = bottom_max6675.sample();

        if (top_adc != 0xFFFF && top_adc != 0xFFFE)
            uint16_t avg = adc_topHeater.reading(top_adc);
        if (bottom_adc != 0xFFFF && bottom_adc != 0xFFFE)
            uint16_t avg = adc_bottomHeater.reading(bottom_adc);

        static int counter = 0;
        counter++;
        if (counter >= 200)
        {
            counter = 0;
            topHeaterPV = MAX6675::toCelcius(adc_topHeater.getAvg());
            bottomHeaterPV = MAX6675::toCelcius(adc_bottomHeater.getAvg());
            if (startedAuto || startedManual)
            {
                secondsRunning++;
            }
        }

        lv_task_handler();
        sleep_ms(5);
    }
    return 0;
}