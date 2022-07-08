#include "HC595.h"
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
#include "pico/multicore.h"
#include "pico/sync.h"
#include "pid.h"
#include <tusb.h>

#define HIGH 1
#define LOW 0

HC595 sft(SFT_DATA, SFT_LATCH, SFT_CLOCK);
MAX6675 top_max6675(THERM_DATA, THERM_SCK, THERM_CS);
MAX6675 bottom_max6675(THERM_DATA, THERM_SCK, UART0_RX);
movingAvg adc_topHeater(10);
movingAvg adc_bottomHeater(10);

// Read-only pointers
static uint32_t bottomHeaterPV = 0;
static uint32_t topHeaterPV = 0;
static uint32_t secondsRunning = 0;

// Read and write pointers
static uint32_t bottomHeaterSV = 70;
static uint32_t topHeaterSV = 150;
static bool startedAuto;
static bool startedManual;
static float topHeaterPID[3];
static float bottomHeaterPID[3];
static uint16_t selectedProfile = 0;
static Profile profileLists[10];

static mutex_t reflow_mutex;

void blinkStatusLED();
void multicore_core1();

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

    // INIT_CRITICAL_SECTION;
    mutex_init(&reflow_mutex);
    sleep_ms(1);
    multicore_launch_core1(multicore_core1);
    sleep_ms(1);

    init_display();
    lv_app_entry();

    while (true)
    {
        static absolute_time_t thread1_abt;
        uint16_t top_adc = top_max6675.sample();
        uint16_t bottom_adc = bottom_max6675.sample();

        if (top_adc != 0xFFFF && top_adc != 0xFFFE)
            uint16_t avg = adc_topHeater.reading(top_adc);
        if (bottom_adc != 0xFFFF && bottom_adc != 0xFFFE)
            uint16_t avg = adc_bottomHeater.reading(bottom_adc);

        if (absolute_time_diff_us(thread1_abt, get_absolute_time()) >= 1000000UL)
        {
            thread1_abt = get_absolute_time();
            mutex_enter_blocking(&reflow_mutex);
            topHeaterPV = MAX6675::toCelcius(adc_topHeater.getAvg());
            bottomHeaterPV = MAX6675::toCelcius(adc_bottomHeater.getAvg());
            if (startedAuto || startedManual)
                secondsRunning++;
            mutex_exit(&reflow_mutex);
        }
        lv_task_handler();
        sleep_ms(5);
    }
    return 0;
}

volatile uint16_t ssr0_pwm = 100;
volatile uint16_t pwm_counter = 0;
struct repeating_timer pwm_timer;
struct repeating_timer reflow_timer;
PID bottomPID;
void multicore_core1()
{
    mutex_init(&reflow_mutex);
    sleep_ms(1);
    static constexpr uint16_t pwm_resolution = 1000;
    sft.init();
    add_repeating_timer_us(
        -500,
        [](repeating_timer_t *rt) -> bool {
            pwm_counter++;
            if (pwm_counter >= ssr0_pwm && ssr0_pwm != pwm_resolution)
                sft.writePin(SFTO::SSR0, 0);
            if (pwm_counter >= pwm_resolution)
            {
                if (ssr0_pwm != 0)
                    sft.writePin(SFTO::SSR0, 1);
                pwm_counter = 0;
            }
            return true;
        },
        NULL, &pwm_timer);

    add_repeating_timer_ms(
        -1000,
        [](repeating_timer_t *rt) -> bool {
            static constexpr uint32_t manualTargetSecond = 90;
            static uint32_t copyTopHeaterPV;
            static uint32_t copyBottomHeaterPV;
            static uint32_t copyTopHeaterSV;
            static uint32_t copyBottomHeaterSV;
            static uint32_t copySecondsRunning;
            static uint32_t lastSecond;
            static float ctopHeaterPID[3];
            static float cbottomHeaterPID[3];
            static float desiredTemperature;
            static float temperatureStep;
            static bool lastStartedManual;
            static bool cStartedManual;
            static float startTemp;

            mutex_enter_blocking(&reflow_mutex);
            memcpy(ctopHeaterPID, topHeaterPID, sizeof(topHeaterPID));
            memcpy(cbottomHeaterPID, bottomHeaterPID, sizeof(bottomHeaterPID));
            copyTopHeaterPV = topHeaterPV;
            copyBottomHeaterPV = bottomHeaterPV;
            copyTopHeaterSV = topHeaterSV;
            copyBottomHeaterSV = bottomHeaterSV;
            copySecondsRunning = secondsRunning;
            cStartedManual = startedManual;
            mutex_exit(&reflow_mutex);

            if (lastStartedManual != cStartedManual && cStartedManual == 1)
            {
                startTemp = copyBottomHeaterPV;
                temperatureStep = (copyBottomHeaterSV - startTemp) / (float)manualTargetSecond;
                desiredTemperature = startTemp + (temperatureStep * (float)copySecondsRunning);
                ssr0_pwm = 0;
                bottomPID.reset();
                printf("step : %.2f C, P %f I %f D %f\n", temperatureStep, bottomHeaterPID[0], bottomHeaterPID[1],
                       bottomHeaterPID[2]);
            }

            if (copySecondsRunning < manualTargetSecond && cStartedManual)
                desiredTemperature = startTemp + (temperatureStep * (float)copySecondsRunning);
            else
                desiredTemperature = copyBottomHeaterSV;

            if (cStartedManual)
            {
                if(copyBottomHeaterPV < desiredTemperature)
                    ssr0_pwm = 1000;
                else if(copyBottomHeaterPV >= desiredTemperature)
                    ssr0_pwm = 0;
                // ssr0_pwm = bottomPID.update(bottomHeaterPID[0], bottomHeaterPID[1], bottomHeaterPID[2],
                //                             desiredTemperature, (float)copyBottomHeaterPV);
                printf("%d;%d;%d\n", copySecondsRunning, ssr0_pwm, copyBottomHeaterPV);
            }
            else
                ssr0_pwm = 0;

            lastSecond = copySecondsRunning;
            lastStartedManual = cStartedManual;
            return true;
        },
        NULL, &reflow_timer);
    // init_display();
    // lv_app_entry();
    while (true)
    {
        tight_loop_contents();
    }
}

void blinkStatusLED()
{
    static int counter = 0;
    counter++;
    if (counter >= 10)
    {
        sft.writePin(SFTO::ST_LED, !sft.readPin(SFTO::ST_LED));
        counter = 0;
    }
}