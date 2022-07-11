#ifndef _PID_H_
#include "pico/platform.h"
#include <stdio.h>
#define _PID_H_
class PID
{

  public:
// Constants used in some of the functions below
#define AUTOMATIC 1
#define MANUAL 0
#define DIRECT 0
#define REVERSE 1
#define P_ON_M 0
#define P_ON_E 1

    // commonly used functions **************************************************************************
    PID(double *, uint16_t *,              // * constructor.  links the PID to the Input, Output, and
        double, double, double, int, int); //   Setpoint.  Initial tuning parameters are also set here.
                                           //   (overload for specifying proportional mode)

    PID(double *, uint16_t *,         // * constructor.  links the PID to the Input, Output, and
        double, double, double, int); //   Setpoint.  Initial tuning parameters are also set here

    PID()
    {
    }

    void init(double *Input, uint16_t *Output, double Kp, double Ki, double Kd, int POn, int ControllerDirection)
    {
        myOutput = Output;
        myInput = Input;
        inAuto = false;
        PID::SetOutputLimits(0., 1000.); // default output limit corresponds to
                                         // the arduino pwm limits

        SampleTime = 1000; // default Controller Sample Time is 0.1 seconds

        PID::SetControllerDirection(MANUAL);
        PID::SetTunings(0, 0, 0);
    }

    void SetMode(int Mode); // * sets PID to either Manual (0) or Auto (non-0)

    bool Compute(double sv); // * performs the PID calculation.  it should be
                             //   called every time loop() cycles. ON/OFF and
                             //   calculation frequency can be set using SetMode
                             //   SetSampleTime respectively

    void SetOutputLimits(double, double); // * clamps the output to a specific range. 0-255 by default, but
                                          //   it's likely the user will want to change this depending on
                                          //   the application

    // available but not commonly used functions ********************************************************
    void SetTunings(double, double, // * While most users will set the tunings once in the
                    double);        //   constructor, this function gives the user the option
                                    //   of changing tunings during runtime for Adaptive control
    void SetTunings(double, double, // * overload for specifying proportional mode
                    double, int);

    void SetControllerDirection(int); // * Sets the Direction, or "Action" of the controller. DIRECT
                                      //   means the output will increase when error is positive. REVERSE
                                      //   means the opposite.  it's very unlikely that this will be needed
                                      //   once it is set in the constructor.
    void SetSampleTime(int);          // * sets the frequency, in Milliseconds, with which
                                      //   the PID calculation is performed.  default is 100

    // Display functions ****************************************************************
    double GetKp();     // These functions query the pid for interal values.
    double GetKi();     //  they were created mainly for the pid front-end,
    double GetKd();     // where it's important to know what is actually
    int GetMode();      //  inside the PID.
    int GetDirection(); //

    void Reset()
    {
        outputSum = 0;
        lastInput = 0;
    };

  private:
    void Initialize();

    double dispKp; // * we'll hold on to the tuning parameters in user-entered
    double dispKi; //   format for display purposes
    double dispKd; //

    double kp; // * (P)roportional Tuning Parameter
    double ki; // * (I)ntegral Tuning Parameter
    double kd; // * (D)erivative Tuning Parameter

    int controllerDirection;
    int pOn;

    double *myInput;    // * Pointers to the Input, Output, and Setpoint variables
    uint16_t *myOutput; //   This creates a hard link between the variables and the

    double outputSum, lastInput;

    unsigned long SampleTime;
    double outMin, outMax;
    bool inAuto, pOnE;
};
#endif