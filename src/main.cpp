#include "FreeRTOS.h"
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
#include "pid.h"
#include "semphr.h"
#include "task.h"
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

static constexpr UBaseType_t lv_app_task_priority = (tskIDLE_PRIORITY + 1);
static constexpr UBaseType_t sensor_task_priority = (tskIDLE_PRIORITY + 2);
static constexpr UBaseType_t pid_task_priority = (tskIDLE_PRIORITY + 3);
static constexpr UBaseType_t pwm_task_priority = (tskIDLE_PRIORITY + 4);

void blinkStatusLED();
static void lv_app_task(void *pvParameter);
static void sensor_task(void *pvParameter);
static void pwm_task(void *pvParameter);
static void pid_task(void *pvParameter);

int main()
{
    stdio_init_all();
    while (!tud_cdc_connected())
        sleep_ms(100);
    if (!set_sys_clock_khz(cpu_freq_mhz * 1000, false))
        printf("set system clock to %dMHz failed\n", cpu_freq_mhz);
    else
        printf("system clock is now %dMHz\n", cpu_freq_mhz);

#ifdef FREE_RTOS_KERNEL_SMP
    printf("Using FreeRTOS SMP Kernel\n");
#endif

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

    xTaskCreate(lv_app_task, "lv_app_task", 4096UL, NULL, lv_app_task_priority, NULL);
    // xTaskCreate(sensor_task, "sensor_task", configMINIMAL_STACK_SIZE, NULL, sensor_task_priority, NULL);
    // xTaskCreate(pid_task, "pid_task", 2048, NULL, pid_task_priority, NULL);
    // xTaskCreate(pwm_task, "pwm_task", configMINIMAL_STACK_SIZE, NULL, pwm_task_priority, NULL);

    vTaskStartScheduler();

    mutex_init(&reflow_mutex);

    while (true)
    {
        /*
        mutex_enter_blocking(&reflow_mutex);
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
            topHeaterPV = MAX6675::toCelcius(adc_topHeater.getAvg());
            bottomHeaterPV = MAX6675::toCelcius(adc_bottomHeater.getAvg());
            if (startedAuto || startedManual)
                secondsRunning++;
        }
        mutex_exit(&reflow_mutex);
        lv_task_handler();
        sleep_ms(5);*/
    }
    return 0;
}

uint16_t ssr0_pwm = 100;
uint16_t pwm_counter = 0;
struct repeating_timer pwm_timer;
struct repeating_timer reflow_timer;
static double copyTopHeaterPV;
static double copyBottomHeaterPV;
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
PID bottomPID;

static void lv_app_task(void *pvParameter)
{
    init_display();
    lv_app_entry();
    for (;;)
    {
        lv_task_handler();
        vTaskDelay(5);
    }
}

static void sensor_task(void *pvParameter)
{
    for (;;)
    {
    }
}

static void pwm_task(void *pvParameter)
{
    for (;;)
    {
    }
}

static void pid_task(void *pvParameter)
{
    for (;;)
    {
    }
}

void multicore_core1()
{
    mutex_init(&reflow_mutex);
    sleep_ms(1);
    static constexpr uint16_t pwm_resolution = 1000;
    sft.init();

    mutex_enter_blocking(&reflow_mutex);
    memcpy(ctopHeaterPID, topHeaterPID, sizeof(topHeaterPID));
    memcpy(cbottomHeaterPID, bottomHeaterPID, sizeof(bottomHeaterPID));
    copyTopHeaterPV = MAX6675::toCelcius(adc_topHeater.getAvg());
    copyBottomHeaterPV = MAX6675::toCelcius(adc_bottomHeater.getAvg());
    copyTopHeaterSV = topHeaterSV;
    copyBottomHeaterSV = bottomHeaterSV;
    copySecondsRunning = secondsRunning;
    cStartedManual = startedManual;
    mutex_exit(&reflow_mutex);

    bottomPID.init(&copyBottomHeaterPV, &ssr0_pwm, cbottomHeaterPID[0], cbottomHeaterPID[1], cbottomHeaterPID[2],
                   P_ON_E, DIRECT);
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

            mutex_enter_blocking(&reflow_mutex);
            memcpy(ctopHeaterPID, topHeaterPID, sizeof(topHeaterPID));
            memcpy(cbottomHeaterPID, bottomHeaterPID, sizeof(bottomHeaterPID));
            copyTopHeaterPV = MAX6675::toCelcius(adc_topHeater.getAvg());
            copyBottomHeaterPV = MAX6675::toCelcius(adc_bottomHeater.getAvg());
            copyTopHeaterSV = topHeaterSV;
            copyBottomHeaterSV = bottomHeaterSV;
            copySecondsRunning = secondsRunning;
            cStartedManual = startedManual;

            if (lastStartedManual != cStartedManual && cStartedManual == 1)
            {
                startTemp = copyBottomHeaterPV;
                temperatureStep = (copyBottomHeaterSV - startTemp) / (float)manualTargetSecond;
                desiredTemperature = startTemp + (temperatureStep * (float)copySecondsRunning);
                bottomPID.SetSampleTime(1000);
                bottomPID.SetTunings(cbottomHeaterPID[0], cbottomHeaterPID[1], cbottomHeaterPID[2], P_ON_E);
                bottomPID.SetOutputLimits(0., 1.);
                bottomPID.SetControllerDirection(DIRECT);
                bottomPID.SetMode(AUTOMATIC);
                bottomPID.Reset();
                ssr0_pwm = 0;
                printf("Starting P %f I %f D %f\n", bottomPID.GetKp(), bottomPID.GetKi(), bottomPID.GetKd());
            }

            if (copySecondsRunning < manualTargetSecond && cStartedManual)
                desiredTemperature = startTemp + (temperatureStep * (float)copySecondsRunning);
            else
                desiredTemperature = copyBottomHeaterSV;

            if (cStartedManual)
            {
                printf("%d;", copySecondsRunning);
                bottomPID.Compute((double)copyBottomHeaterSV);
            }
            else
            {
                bottomPID.SetMode(MANUAL);
                ssr0_pwm = 0;
            }
            lastSecond = copySecondsRunning;
            lastStartedManual = cStartedManual;
            mutex_exit(&reflow_mutex);
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