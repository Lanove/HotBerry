#include "pid.h"

/*Constructor (...)*********************************************************
 *    The parameters specified here are those for for which we can't set up
 *    reliable defaults, so we need to have the user set them.
 ***************************************************************************/
PID::PID(double *Input, uint16_t *Output, double Kp, double Ki, double Kd, int POn, int ControllerDirection)
{
    myOutput = Output;
    myInput = Input;
    inAuto = false;

    PID::SetOutputLimits(0, 255); // default output limit corresponds to
                                  // the arduino pwm limits

    SampleTime = 100; // default Controller Sample Time is 0.1 seconds

    PID::SetControllerDirection(ControllerDirection);
    PID::SetTunings(Kp, Ki, Kd, POn);
}

/*Constructor (...)*********************************************************
 *    To allow backwards compatability for v1.1, or for people that just want
 *    to use Proportional on Error without explicitly saying so
 ***************************************************************************/

PID::PID(double *Input, uint16_t *Output, double Kp, double Ki, double Kd, int ControllerDirection)
    : PID::PID(Input, Output, Kp, Ki, Kd, P_ON_E, ControllerDirection)
{
}

/* Compute() **********************************************************************
 *     This, as they say, is where the magic happens.  this function should be called
 *   every time "void loop()" executes.  the function will decide for itself whether a new
 *   pid Output needs to be computed.  returns true when the output is computed,
 *   false when nothing has been done.
 **********************************************************************************/
bool PID::Compute(double sv)
{
    if (!inAuto)
        return false;
    /*Compute all the working error variables*/
    double input = (double)*myInput;
    double error = sv - input;
    outputSum += (ki * error);

    /*Integral anti wind-up by clamping*/
    if (outputSum > outMax)
        outputSum = outMax;
    else if (outputSum < outMin)
        outputSum = outMin;

    double output;
    output = kp * error;

    /*Compute Rest of PID Output*/
    output += outputSum + kd * (error - lastError);

    /* Output clamping */
    if (output > outMax)
        output = outMax;
    else if (output < outMin)
        output = outMin;
    *myOutput = (uint16_t)(output * 1000.);

    printf("%.2f;%.3f;%.3f;%.3f;%.3f;%.3f;%.2f\n", *myInput, output, error, kd * (error - lastError), kp * error, outputSum, sv);
    /*Remember some variables for next time*/
    lastInput = input;
    lastError = error;
    return true;
}

/* SetTunings(...)*************************************************************
 * This function allows the controller's dynamic performance to be adjusted.
 * it's called automatically from the constructor, but tunings can also
 * be adjusted on the fly during normal operation
 ******************************************************************************/
void PID::SetTunings(double Kp, double Ki, double Kd, int POn)
{
    if (Kp < 0 || Ki < 0 || Kd < 0)
        return;

    pOn = POn;
    pOnE = POn == P_ON_E;

    dispKp = Kp;
    dispKi = Ki;
    dispKd = Kd;

    double SampleTimeInSec = ((double)SampleTime) / 1000;
    kp = Kp;
    ki = Ki * SampleTimeInSec;
    kd = Kd / SampleTimeInSec;

    if (controllerDirection == REVERSE)
    {
        kp = (0 - kp);
        ki = (0 - ki);
        kd = (0 - kd);
    }
}

/* SetTunings(...)*************************************************************
 * Set Tunings using the last-rembered POn setting
 ******************************************************************************/
void PID::SetTunings(double Kp, double Ki, double Kd)
{
    SetTunings(Kp, Ki, Kd, pOn);
}

/* SetSampleTime(...) *********************************************************
 * sets the period, in Milliseconds, at which the calculation is performed
 ******************************************************************************/
void PID::SetSampleTime(int NewSampleTime)
{
    if (NewSampleTime > 0)
    {
        double ratio = (double)NewSampleTime / (double)SampleTime;
        ki *= ratio;
        kd /= ratio;
        SampleTime = (unsigned long)NewSampleTime;
    }
}

/* SetOutputLimits(...)****************************************************
 *     This function will be used far more often than SetInputLimits.  while
 *  the input to the controller will generally be in the 0-1023 range (which is
 *  the default already,)  the output will be a little different.  maybe they'll
 *  be doing a time window and will need 0-8000 or something.  or maybe they'll
 *  want to clamp it from 0-125.  who knows.  at any rate, that can all be done
 *  here.
 **************************************************************************/
void PID::SetOutputLimits(double Min, double Max)
{
    if (Min >= Max)
        return;
    outMin = Min;
    outMax = Max;

    if (inAuto)
    {
        if (*myOutput > outMax)
            *myOutput = outMax;
        else if (*myOutput < outMin)
            *myOutput = outMin;

        if (outputSum > outMax)
            outputSum = outMax;
        else if (outputSum < outMin)
            outputSum = outMin;
    }
}

/* SetMode(...)****************************************************************
 * Allows the controller Mode to be set to manual (0) or Automatic (non-zero)
 * when the transition from manual to auto occurs, the controller is
 * automatically initialized
 ******************************************************************************/
void PID::SetMode(int Mode)
{
    bool newAuto = (Mode == AUTOMATIC);
    if (newAuto && !inAuto)
    { /*we just went from manual to auto*/
        PID::Initialize();
    }
    inAuto = newAuto;
}

/* Initialize()****************************************************************
 *	does all the things that need to happen to ensure a bumpless transfer
 *  from manual to automatic mode.
 ******************************************************************************/
void PID::Initialize()
{
    outputSum = *myOutput;
    lastInput = (double)*myInput;
    if (outputSum > outMax)
        outputSum = outMax;
    else if (outputSum < outMin)
        outputSum = outMin;
}

/* SetControllerDirection(...)*************************************************
 * The PID will either be connected to a DIRECT acting process (+Output leads
 * to +Input) or a REVERSE acting process(+Output leads to -Input.)  we need to
 * know which one, because otherwise we may increase the output when we should
 * be decreasing.  This is called from the constructor.
 ******************************************************************************/
void PID::SetControllerDirection(int Direction)
{
    if (inAuto && Direction != controllerDirection)
    {
        kp = (0 - kp);
        ki = (0 - ki);
        kd = (0 - kd);
    }
    controllerDirection = Direction;
}

/* Status Funcions*************************************************************
 * Just because you set the Kp=-1 doesn't mean it actually happened.  these
 * functions query the internal state of the PID.  they're here for display
 * purposes.  this are the functions the PID Front-end uses for example
 ******************************************************************************/
double PID::GetKp()
{
    return dispKp;
}
double PID::GetKi()
{
    return dispKi;
}
double PID::GetKd()
{
    return dispKd;
}
int PID::GetMode()
{
    return inAuto ? AUTOMATIC : MANUAL;
}
int PID::GetDirection()
{
    return controllerDirection;
}