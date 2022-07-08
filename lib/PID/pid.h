#ifndef _PID_H_
#include "pico/platform.h"
#include <stdio.h>
#define _PID_H_
class PID
{
  public:
    typedef float variable_t;
    uint16_t update(variable_t kp, variable_t ki, variable_t kd, variable_t desiredTemperature,
                    variable_t currentTemperature);
    void reset()
    {
        _lastError = 0.;
        _integral = 0.;
    };

  private:
    variable_t _lastError = 0.;
    variable_t _integral = 0.;
};
#endif