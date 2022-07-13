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

static constexpr UBaseType_t lv_app_task_priority = (tskIDLE_PRIORITY + 1);
static constexpr UBaseType_t sensor_task_priority = (tskIDLE_PRIORITY + 2);
static constexpr UBaseType_t pid_task_priority = (tskIDLE_PRIORITY + 3);

static TaskHandle_t lv_app_task_handle;
static TaskHandle_t sensor_task_handle;
static TaskHandle_t pid_task_handle;

static SemaphoreHandle_t sensor_mutex;

void blinkStatusLED();
static void lv_app_task(void *pvParameter);
static void sensor_task(void *pvParameter);
static void pid_task(void *pvParameter);

bool pwm_timer_cb(repeating_timer_t *rt);
static constexpr uint16_t pwm_resolution = 1000;
static QueueHandle_t pwm_ssr0_queue = NULL;
uint16_t pwm_ssr0 = 100;
uint16_t pwm_ssr1 = 100;
uint16_t pwm_counter = 0;
struct repeating_timer pwm_timer;
PIDController PID_bottomHeater;

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

    sft.init();

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

    LV_APP_MUTEX_INIT;
    sensor_mutex = xSemaphoreCreateMutex();
    pwm_ssr0_queue = xQueueCreate(1, sizeof(uint32_t));

    if (pwm_ssr0_queue == NULL)
        printf("failed creating queue\n");
    else
        printf("success creating queue\n");

    add_repeating_timer_us(-500, pwm_timer_cb, NULL, &pwm_timer);

    xTaskCreate(lv_app_task, "lv_app_task", 8192UL, NULL, lv_app_task_priority, &lv_app_task_handle);
    xTaskCreate(sensor_task, "sensor_task", configMINIMAL_STACK_SIZE, NULL, sensor_task_priority, &sensor_task_handle);
    xTaskCreate(pid_task, "pid_task", 1024UL, NULL, pid_task_priority, &pid_task_handle);

    vTaskStartScheduler();

    while (true)
        ;
    return 0;
}

bool pwm_timer_cb(repeating_timer_t *rt)
{
    static uint16_t c_pwm_ssr0 = 0;
    static uint16_t c_pwm_ssr1 = 0;

    xQueueReceiveFromISR(pwm_ssr0_queue, &c_pwm_ssr0, NULL);

    pwm_counter++;
    if (pwm_counter >= c_pwm_ssr0 && c_pwm_ssr0 != pwm_resolution)
        sft.writePin(SFTO::SSR0, 0);
    if (pwm_counter >= c_pwm_ssr1 && c_pwm_ssr1 != pwm_resolution)
        sft.writePin(SFTO::SSR1, 0);
    if (pwm_counter >= pwm_resolution)
    {
        if (c_pwm_ssr0 != 0)
            sft.writePin(SFTO::SSR0, 1);
        if (c_pwm_ssr1 != 0)
            sft.writePin(SFTO::SSR1, 1);
        pwm_counter = 0;
    }
    return true;
}

static void lv_app_task(void *pvParameter)
{
    init_display();
    lv_app_entry();
    for (;;)
    {
        lv_task_handler();
        vTaskDelay(5 / portTICK_PERIOD_MS);
    }
}

static void sensor_task(void *pvParameter)
{
    top_max6675.init();
    bottom_max6675.init();
    adc_topHeater.begin();
    adc_bottomHeater.begin();
    for (;;)
    {
        uint16_t top_adc = top_max6675.sample();
        uint16_t bottom_adc = bottom_max6675.sample();
        xSemaphoreTake(sensor_mutex, portMAX_DELAY);
        adc_topHeater.reading(top_adc);
        adc_bottomHeater.reading(bottom_adc);
        xSemaphoreGive(sensor_mutex);
        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
}

static void pid_task(void *pvParameter)
{
    static constexpr float PID_sampleTime = 1.0f;
    static constexpr float PID_derivativeTau = .3f; // Pole at 3.333 or 1/0.3
    bool lastStartedManual;
    PIDController_Init(&PID_bottomHeater);
    PIDController_SetIntegralLimit(&PID_bottomHeater, 0.0f, 1.0f);
    PIDController_SetOutputLimit(&PID_bottomHeater, 0.0f, 1.0f);
    for (;;)
    {
        xSemaphoreTake(sensor_mutex, portMAX_DELAY);
        float topHeaterPV_f = MAX6675::toCelcius(adc_topHeater.getAvg());
        float bottomHeaterPV_f = MAX6675::toCelcius(adc_bottomHeater.getAvg());
        xSemaphoreGive(sensor_mutex);

        xSemaphoreTake(lv_app_mutex, portMAX_DELAY);
        topHeaterPV = topHeaterPV_f;
        bottomHeaterPV = bottomHeaterPV_f;
        if (startedAuto || startedManual)
            secondsRunning++;

        if (lastStartedManual != startedManual && startedManual == 1)
        {
            PIDController_Init(&PID_bottomHeater);
            pwm_ssr0 = 0;
            PIDController_SetTuning(&PID_bottomHeater, bottomHeaterPID[0], bottomHeaterPID[1], bottomHeaterPID[2],
                                    PID_sampleTime, PID_derivativeTau);
            PID_bottomHeater.prevMeasurement = bottomHeaterPV_f;
            printf("Starting P %f I %f D %f sampleTime %.1f tau %f\n", PID_bottomHeater.Kp, PID_bottomHeater.Ki,
                   PID_bottomHeater.Kd, PID_sampleTime, PID_derivativeTau);
        }

        if (startedManual)
        {
            PIDController_Compute(&PID_bottomHeater, (pid_variable_t)bottomHeaterSV, (pid_variable_t)bottomHeaterPV_f);
            // seconds;pv;output;p;i;d;error;sv
            printf("%d;%.2f;%.3f;%.3f;%.3f;%.3f;%.2f;%.2f\n", secondsRunning, PID_bottomHeater.prevMeasurement,
                   PID_bottomHeater.out, PID_bottomHeater.proportional, PID_bottomHeater.integrator,
                   PID_bottomHeater.differentiator, PID_bottomHeater.prevError, PID_bottomHeater.setPoint);
        }

        if (startedManual)
            pwm_ssr0 = (uint16_t)(PID_bottomHeater.out * 1000.);
        else
            pwm_ssr0 = 0;

        xQueueSend(pwm_ssr0_queue, &pwm_ssr0, portMAX_DELAY);

        lastStartedManual = startedManual;
        xSemaphoreGive(lv_app_mutex);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}