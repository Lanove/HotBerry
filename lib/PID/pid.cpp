#include "pid.h"

void PIDController_SetOutputLimit(PIDController *pid, pid_variable_t limMin, pid_variable_t limMax)
{
    pid->limMin = limMin;
    pid->limMax = limMax;
}

void PIDController_SetIntegralLimit(PIDController *pid, pid_variable_t limMin, pid_variable_t limMax)
{
    pid->limMinInt = limMin;
    pid->limMaxInt = limMax;
}

void PIDController_SetTuning(PIDController *pid, pid_variable_t kp, pid_variable_t ki, pid_variable_t kd,
                             pid_variable_t sampleTime, pid_variable_t tau)
{
    pid->Kp = kp;
    pid->Ki = ki;
    pid->Kd = kd;
    pid->T = sampleTime;
    pid->tau = 0.00;
}

void PIDController_Init(PIDController *pid)
{

    /* Clear controller variables */
    pid->integrator = 0.0f;
    pid->prevError = 0.0f;
    pid->proportional = 0.0f;

    pid->differentiator = 0.0f;
    pid->prevMeasurement = 0.0f;

    pid->out = 0.0f;
}

pid_variable_t PIDController_Compute(PIDController *pid, pid_variable_t setpoint, pid_variable_t measurement)
{

    pid_variable_t error = setpoint - measurement;
    pid_variable_t proportional = pid->Kp * error;
    pid->proportional = proportional;
    pid->setPoint = setpoint;

    pid->integrator = pid->integrator + 0.5f * pid->Ki * pid->T * (error + pid->prevError);

    /* Anti-wind-up via integrator clamping */
    if (pid->integrator > pid->limMaxInt)
        pid->integrator = pid->limMaxInt;
    else if (pid->integrator < pid->limMinInt)
        pid->integrator = pid->limMinInt;

    /*
    pid->differentiator =
        -(2.0f * pid->Kd * (measurement - pid->prevMeasurement) + (2.0f * pid->tau - pid->T) * pid->differentiator) /
        (2.0f * pid->tau + pid->T);
    */

    pid->differentiator = -((pid->Kd / pid->T) * (measurement - pid->prevMeasurement));

    /*
     * Compute output and apply limits
     */
    pid->out = proportional + pid->integrator + pid->differentiator;

    if (pid->out > pid->limMax)
        pid->out = pid->limMax;
    else if (pid->out < pid->limMin)
        pid->out = pid->limMin;

    /* Store error and measurement for later use */
    pid->prevError = error;
    pid->prevMeasurement = measurement;

    /* Return controller output */
    return pid->out;
}