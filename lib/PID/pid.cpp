#include "pid.h"

uint16_t PID::update(variable_t kp, variable_t ki, variable_t kd, variable_t desiredTemperature,
                     variable_t currentTemperature)
{
    variable_t error, pwm, derivative;

    // current error term is the difference between desired and current temperature
    error = desiredTemperature - currentTemperature;

    // update the integral (historical error)
    _integral += error;

    // the derivative term
    derivative = error - _lastError;

    // calculate the control variable
    pwm = (kp * error) + (ki * _integral) + (kd * derivative);
    pwm = MAX(MIN(1000.0, pwm), 0.0);

    // save the last error
    _lastError = error;

    // return the control variable
    return static_cast<uint16_t>(pwm);
}