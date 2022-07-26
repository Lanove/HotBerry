/**
 * @file main.cpp
 * @author figoarzaki123@gmail.com / 5022221041@mhs.its.ac.id
 * @brief This is the main program for HotBerry, The program runs using FreeRTOS SMP Kernel
 * @version 1.0
 * @date 2022-07-26
 *
 * @copyright Copyright (c) 2022
 * @todo Auto operation is not yet implemented because the profile graph
 * somehow bugged and not drawn when the profile total second is >= 373
 */

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

void blinkStatusLED();
static void lv_app_task(void *pvParameter);
static void sensor_task(void *pvParameter);
static void pid_task(void *pvParameter);

HC595 sft(SFT_DATA, SFT_LATCH, SFT_CLOCK);
MAX6675 top_max6675(THERM_DATA, THERM_SCK, THERM_CS);
MAX6675 bottom_max6675(THERM_DATA, THERM_SCK, UART0_RX);
movingAvg adc_topHeater(10);
movingAvg adc_bottomHeater(10);
PIDController PID_bottomHeater, PID_topHeater;

static uint32_t bottomHeaterPV = 0;
static uint32_t topHeaterPV = 0;
static uint32_t bottomHeaterSV = 300;
static uint32_t topHeaterSV = 0;

static uint32_t secondsRunning = 0;
static bool startedAuto;
static bool startedManual;
static uint16_t selectedProfile = 0;
static Profile profileLists[10];

// Variable to store PID constants, P I D and tau consecutively
static double topHeaterPID[4];
static double bottomHeaterPID[4];

static constexpr UBaseType_t lv_app_task_priority = (tskIDLE_PRIORITY + 1);
static constexpr UBaseType_t sensor_task_priority = (tskIDLE_PRIORITY + 2);
static constexpr UBaseType_t pid_task_priority = (tskIDLE_PRIORITY + 3);
static constexpr uint32_t lv_app_task_stack_size = 8192UL;
static constexpr uint32_t sensor_task_stack_size = configMINIMAL_STACK_SIZE;
static constexpr uint32_t pid_task_stack_size = 1024UL;
static TaskHandle_t lv_app_task_handle;
static TaskHandle_t sensor_task_handle;
static TaskHandle_t pid_task_handle;
static SemaphoreHandle_t sensor_mutex;

bool pwm_timer_cb(repeating_timer_t *rt);
static constexpr uint16_t pwm_resolution = 1000;
static constexpr int64_t pwm_period = -500; // in microseconds, negative to make the timer repeated infinitely
static QueueHandle_t pwm_ssr0_queue = NULL;
static QueueHandle_t pwm_ssr1_queue = NULL;
uint16_t pwm_ssr0;
uint16_t pwm_ssr1;
uint16_t pwm_counter;
struct repeating_timer pwm_timer;

int main()
{
    stdio_init_all();

    // Initialize shift register (only used for SSR PWM)
    sft.init();

    // Initializer pointers used for lv_app
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

    // Initialize mutex and queues used later for RTOS tasks
    lv_app_mutex = xSemaphoreCreateMutex();
    sensor_mutex = xSemaphoreCreateMutex();
    pwm_ssr0_queue = xQueueCreate(1, sizeof(uint32_t));
    pwm_ssr1_queue = xQueueCreate(1, sizeof(uint32_t));

    add_repeating_timer_us(pwm_period, pwm_timer_cb, NULL, &pwm_timer);

    // Create RTOS tasks
    xTaskCreate(lv_app_task, "lv_app_task", lv_app_task_stack_size, NULL, lv_app_task_priority, &lv_app_task_handle);
    xTaskCreate(sensor_task, "sensor_task", sensor_task_stack_size, NULL, sensor_task_priority, &sensor_task_handle);
    xTaskCreate(pid_task, "pid_task", pid_task_stack_size, NULL, pid_task_priority, &pid_task_handle);

    // Start the RTOS scheduler, the created tasks will run after this point
    vTaskStartScheduler();

    while (true) // Do nothing because all stuffs is done on RTOS tasks
        ;
    return 0;
}

bool pwm_timer_cb(repeating_timer_t *rt)
{
    static uint16_t c_pwm_ssr0 = 0;
    static uint16_t c_pwm_ssr1 = 0;

    // Check queue for any data, if there is some data, put it on corresponding c_pwm_ssrx
    xQueueReceiveFromISR(pwm_ssr0_queue, &c_pwm_ssr0, NULL);
    xQueueReceiveFromISR(pwm_ssr1_queue, &c_pwm_ssr1, NULL);

    // Software PWM implementation :
    pwm_counter++;
    // Turn off the output if the counter is exceeding the pwm value, but don't turn the output off if the pwm value is
    // on maximal value
    if (pwm_counter >= c_pwm_ssr0 && c_pwm_ssr0 != pwm_resolution)
        sft.writePin(SFTO::SSR0, 0);
    if (pwm_counter >= c_pwm_ssr1 && c_pwm_ssr1 != pwm_resolution)
        sft.writePin(SFTO::SSR1, 0);

    // Reset the counter if the counter exceed maximal value or resolution
    // Also reset the output by setting it to HIGH
    if (pwm_counter >= pwm_resolution)
    {
        pwm_counter = 0;
        if (c_pwm_ssr0 != 0)
            sft.writePin(SFTO::SSR0, 1);
        if (c_pwm_ssr1 != 0)
            sft.writePin(SFTO::SSR1, 1);
    }

    // must return true, otherwise the timer is not repeated
    return true;
}

// All lvgl and display related task is here
static void lv_app_task(void *pvParameter)
{
    // Initialize ILI9486 display and LVGL
    init_display();
    // Enter the lv_app
    lv_app_entry();

    for (;;)
    {
        lv_task_handler();
        vTaskDelay(5 / portTICK_PERIOD_MS);
    }
}

// Task to sample MAX6675 thermocouple every 200ms (MAX6675 conversion time is 170ms)
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
        // Add the new adc readout to the moving average
        adc_topHeater.reading(top_adc);
        adc_bottomHeater.reading(bottom_adc);
        xSemaphoreGive(sensor_mutex);
        
        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
}

static void pid_task(void *pvParameter)
{
    // Kp = controller gain
    // Ki = w1 * P
    // Kd = P / w2
    static constexpr float PID_sampleTime = 1.0f;
    static constexpr float PID_derivativeTau = 2.161931848f;
    bool lastStartedManual;
    PIDController_Init(&PID_bottomHeater);
    PIDController_SetIntegralLimit(&PID_bottomHeater, 0.f, 1.0f);
    PIDController_SetOutputLimit(&PID_bottomHeater, 0.0f, 1.0f);
    PIDController_Init(&PID_topHeater);
    PIDController_SetIntegralLimit(&PID_topHeater, 0.f, 1.0f);
    PIDController_SetOutputLimit(&PID_topHeater, 0.0f, 1.0f);
    for (;;)
    {
        xSemaphoreTake(sensor_mutex, portMAX_DELAY);
        // Calculate the adc readout to temperature in celcius
        float topHeaterPV_f = MAX6675::toCelcius(adc_topHeater.getAvg());
        float bottomHeaterPV_f = MAX6675::toCelcius(adc_bottomHeater.getAvg());
        xSemaphoreGive(sensor_mutex);

        xSemaphoreTake(lv_app_mutex, portMAX_DELAY);

        // Update the PV that is used for lv_app
        topHeaterPV = topHeaterPV_f;
        bottomHeaterPV = bottomHeaterPV_f;
        
        // If either started flag is true, we increment the secondsRunning
        if (startedAuto || startedManual)
            secondsRunning++;

        // Initialize PID on the rising edge of startedManual flag
        if (lastStartedManual != startedManual && startedManual == 1)
        {
            PIDController_Init(&PID_bottomHeater);
            pwm_ssr0 = 0;
            PIDController_SetTuning(&PID_bottomHeater, bottomHeaterPID[0], bottomHeaterPID[1], bottomHeaterPID[2],
                                    PID_sampleTime, PID_derivativeTau);
            PID_bottomHeater.prevMeasurement = bottomHeaterPV_f;

            PIDController_Init(&PID_topHeater);
            pwm_ssr1 = 0;
            PIDController_SetTuning(&PID_topHeater, topHeaterPID[0], topHeaterPID[1], topHeaterPID[2], PID_sampleTime,
                                    PID_derivativeTau);
            PID_topHeater.prevMeasurement = topHeaterPV_f;

            printf("topHeater P %f I %f D %f sampleTime %.1f tau %f\n", PID_topHeater.Kp, PID_topHeater.Ki,
                   PID_topHeater.Kd, PID_sampleTime, PID_derivativeTau);
            printf("bottomHeater P %f I %f D %f sampleTime %.1f tau %f\n", PID_bottomHeater.Kp, PID_bottomHeater.Ki,
                   PID_bottomHeater.Kd, PID_sampleTime, PID_derivativeTau);
        }

        // Compute the PIDs
        if (startedManual)
        {
            PIDController_Compute(&PID_bottomHeater, (pid_variable_t)bottomHeaterSV, (pid_variable_t)bottomHeaterPV_f);
            PIDController_Compute(&PID_topHeater, (pid_variable_t)topHeaterSV, (pid_variable_t)topHeaterPV_f);
        }

        // Apply PID output to PWM if the startedManual flag is true
        if (startedManual)
            pwm_ssr0 = (uint16_t)(PID_bottomHeater.out * 1000.);
        else // Set the PWM value to 0 if startedManual is false
            pwm_ssr0 = 0;

        if (startedManual)
            pwm_ssr1 = (uint16_t)(PID_topHeater.out * 1000.);
        else
            pwm_ssr1 = 0;

        // Send the current computed PWM value to the queue
        xQueueSend(pwm_ssr0_queue, &pwm_ssr0, portMAX_DELAY);
        xQueueSend(pwm_ssr1_queue, &pwm_ssr1, portMAX_DELAY);

        lastStartedManual = startedManual;
        xSemaphoreGive(lv_app_mutex);

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}