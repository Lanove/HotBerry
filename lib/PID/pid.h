#ifndef _PID_H_
#include "pico/platform.h"
#include <stdio.h>
#define _PID_H_

typedef double pid_variable_t;

typedef struct
{

    /* Controller gains */
    pid_variable_t Kp;
    pid_variable_t Ki;
    pid_variable_t Kd;

    /* Derivative low-pass filter time constant */
    pid_variable_t tau;

    /* Output limits */
    pid_variable_t limMin;
    pid_variable_t limMax;

    /* Integrator limits */
    pid_variable_t limMinInt;
    pid_variable_t limMaxInt;

    /* Sample time (in seconds) */
    pid_variable_t T;

    /* Controller "memory" */
    pid_variable_t setPoint;
    pid_variable_t proportional;
    pid_variable_t integrator;
    pid_variable_t prevError; /* Required for integrator */
    pid_variable_t differentiator;
    pid_variable_t prevMeasurement; /* Required for differentiator */

    /* Controller output */
    pid_variable_t out;
} PIDController;

void PIDController_SetOutputLimit(PIDController *pid, pid_variable_t limMin, pid_variable_t limMax);
void PIDController_SetIntegralLimit(PIDController *pid, pid_variable_t limMin, pid_variable_t limMax);
void PIDController_SetTuning(PIDController *pid, pid_variable_t kp, pid_variable_t ki, pid_variable_t kd,
                             pid_variable_t sampleTime, pid_variable_t tau);
void PIDController_Init(PIDController *pid);
pid_variable_t PIDController_Compute(PIDController *pid, pid_variable_t setpoint, pid_variable_t measurement);
#endif