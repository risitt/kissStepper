/*
Keep it Simple Stepper - a lightweight library for the Easy Driver, Big Easy Driver, Allegro stepper motor drivers and others that use a Step/Dir interface
Written by Rylee Isitt. September 21, 2015
License: GNU Lesser General Public License (LGPL) V2.1

Despite the existence of several excellent libraries for driving stepper motors, I created this one to fullfill the following needs:
* LGPL instead of GPL, so that you can use it in your own project with few licensing restrictions (please read the LGPL V2.1 for details).
* Low memory and processing demands
* Consistent motor speed regardless of drive mode
* Consistent position index, so even after changing drive modes, position 1000 will refer to the same location as before
* Automatic handling of MS1, MS2, and MS3 (microstep select) pins if desired
* Automatic handling of Enable pin if desired (set to 255 if you don't want to use this feature)
* Acceleration for driving heavier loads and reaching higher speeds before the motor stalls
*/
#include "kissStepper.h"

kissStepper::kissStepper(uint16_t motorSteps, kissPinAssignments pinAssignments, kissMicrostepConfig microstepConfig)
    :motorStPerRev(motorSteps),
     pinDir(pinAssignments.pinDir), pinStep(pinAssignments.pinStep), pinEnable(pinAssignments.pinEnable),
     pinMS1(pinAssignments.pinMS1), pinMS2(pinAssignments.pinMS2), pinMS3(pinAssignments.pinMS3),
     maxMicrostepMode(microstepConfig.maxMicrostepMode), MS1Config(microstepConfig.MS1Config), MS2Config(microstepConfig.MS2Config), MS3Config(microstepConfig.MS3Config) {}

// ----------------------------------------------------------------------------------------------------
// Initialize the motor in a default state:
// - Disabled (ENABLE pin high)
// - Set to move forwards (DIR pin low)
// - STEP pin low
// - Default to full step drive mode if none specified by the user
// - Default to 30 RPM if speed not specified by the user
// - Default to no acceleration
// ----------------------------------------------------------------------------------------------------

void kissStepper::begin(driveMode_t mode, uint16_t maxRPM, uint16_t accelRPMS)
{
    // set pins to output
    pinMode(pinDir, OUTPUT);
    pinMode(pinStep, OUTPUT);
    if (pinMS1 != 255) pinMode(pinMS1, OUTPUT);
    if (pinMS2 != 255) pinMode(pinMS2, OUTPUT);
    if (pinMS3 != 255) pinMode(pinMS3, OUTPUT);

    // start the controller in a disabled state
    enabled = false;
    if (pinEnable < 255)
    {
        pinMode(pinEnable, OUTPUT);
        digitalWrite(pinEnable, HIGH);
    }

    // defaults
    digitalWrite(pinDir, LOW); // forwards
    digitalWrite(pinStep, LOW);
    dir = true;
    pos = 0;
    stop();
    forwardLimit = 2147483647L;
    reverseLimit = -2147483648L;
    setDriveMode(mode);
    setMaxRPM(maxRPM);
    setAccel(accelRPMS);

    // this allows us to convert from a standard Arduino pin number to an AVR port
    // for faster digital writes in the work() method at the cost of some memory
    // we don't use this technique for other digitalWrites because they are infrequent
    stepBit = digitalPinToBitMask(pinStep);
    stepOut = portOutputRegister(digitalPinToPort(pinStep));

}

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

void kissStepper::enable(void)
{
    if (pinEnable < 255) digitalWrite(pinEnable, LOW);
    enabled = true;
}

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

void kissStepper::disable(void)
{
    if (pinEnable < 255)
    {
        delay(50); // this short delay stops motor momentum
        digitalWrite(pinEnable, HIGH);
    }
    target = pos;
    enabled = false;
}

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

void kissStepper::setDriveMode(driveMode_t mode)
{
    // do not allow modes beyond the limit set by the user
    if ((uint8_t)mode < (uint8_t)maxMicrostepMode) mode = maxMicrostepMode;

    // set the pin states
    if (pinMS1 < 255) digitalWrite(pinMS1, ((MS1Config & (uint8_t)mode) ? HIGH : LOW));
    if (pinMS2 < 255) digitalWrite(pinMS2, ((MS2Config & (uint8_t)mode) ? HIGH : LOW));
    if (pinMS3 < 255) digitalWrite(pinMS3, ((MS3Config & (uint8_t)mode) ? HIGH : LOW));

    driveMode = mode;
    setCurRP10M(curRP10M);
}

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

void kissStepper::setMaxRPM(uint16_t newMaxRPM)
{
    maxRP10M = (newMaxRPM * 10);
    // if the motor is moving and acceleration is off, change speed immediately
    if ((!accel) && (moving)) setCurRP10M(maxRP10M);
}

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

void kissStepper::setMaxRP10M(uint16_t newMaxRP10M)
{
    maxRP10M = newMaxRP10M;
    // if the motor is moving and acceleration is off, change speed immediately
    if ((!accel) && (moving)) setCurRP10M(newMaxRP10M);
}

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

void kissStepper::setCurRP10M(uint16_t newCurRP10M)
{
    // The 4687500 "magic number" starts with the number of microseconds in 600 seconds (600000000).
    // 600 seconds is used because speed calculations are based on revolutions per 10 minutes (600 seconds)
    // then it is divided by the maximum microstep number (128), and the result is 4687500
    if (newCurRP10M > 0)
    {
        uint32_t numer = 4687500UL * driveMode;
        uint32_t denom = (uint32_t)motorStPerRev * newCurRP10M;
        stepInterval =  numer / denom;
        if ((numer % denom) > (denom >> 1)) stepInterval++;
    }
    else stepInterval = 4294967295UL;
    curRP10M = newCurRP10M;
}

// ----------------------------------------------------------------------------------------------------
// This method sets the motor acceleration in RPM/s
// ----------------------------------------------------------------------------------------------------

bool kissStepper::setAccel(uint16_t RPMS)
{
    // calculate the time interval at which to increment curRP10M
    // and recalculate decelDistance
    // but only allow if not currently accelerating
    // interval is 1/10th what you might expect because it is incrementing RP10M, not RPM
    if (accelState == 0)
    {
        if (RPMS > 0)
        {
            accelInterval = 100000UL / RPMS;
            if ((100000UL % RPMS) > (RPMS >> 1)) accelInterval++;
            decelDistance = (decelDistance*accel) / RPMS;
        }
        accel = RPMS;
        return true;
    }
    return false;
}

// ----------------------------------------------------------------------------------------------------
// Makes the motor work. Call repeatedly and often for smooth motion.
// Returns true if the motor is moving, otherwise false.
// ----------------------------------------------------------------------------------------------------
bool kissStepper::work(void)
{

    // check if it's necessary to move the motor
    // compare to step size to prevent the motor from twitching back and forth around the target position;
    uint32_t stepsRemaining = abs(target - pos);

    if (stepsRemaining >= driveMode)
    {
        uint32_t curTime = micros();

        // set up the step sequence
        // this block must always run once, and only once, at the start of a step sequence
        if (!moving)
        {
            lastStepTime = curTime; // this prevents lastStepTime from lagging behind
            lastAccelTime = curTime; // this prevents lastAccelTime from lagging behind
            if (!accel) setCurRP10M(maxRP10M); // if not accelerating, start motor at full speed
            if (!enabled) enable(); // enable the motor controller if needed
            moving = true;
        }

        // Handle acceleration
        // What happens if curTime rolls over but lastAccelTime has not?
        // In such a situation, curTime < lastAccelTime. Wouldn't that break the timing?
        // Let's see:
        // curTime = 3
        // lastAccelTime = 4294967291
        // curTime - lastAccelTime = -4294967288
        // But with unsigned math (as it is below), you can't get a negative result, so actually
        // curTime - lastAccelTime = 8
        // And it's all accounted for. It's like magic!
        // Adding accelInterval to lastAccelTime produces more accurate timing than setting lastAccelTime = curTime
        if (accel)
        {
            if ((stepsRemaining > decelDistance) && (curRP10M < maxRP10M))   // accelerate
            {
                if (accelState != 1)
                {
                    accelState = 1;
                    lastAccelTime = curTime;
                }
            }
            else if (((curRP10M > 1) && (stepsRemaining < decelDistance)) || (curRP10M > maxRP10M))     // decelerate
            {
                if (accelState != -1)
                {
                    accelState = -1;
                    lastAccelTime = curTime;
                }
            }
            else if (accelState != 0) accelState = 0;

            if ((accelState != 0) && ((curTime - lastAccelTime) >= accelInterval))
            {
                setCurRP10M(curRP10M+accelState);
                lastAccelTime += accelInterval;
            }
        }

        // Step, if it's time...
        // Adding stepInterval to lastStepTime produces more accurate timing than setting lastStepTime = curTime
        if (!(*stepOut & stepBit))
        {
            if ((curTime - lastStepTime) >= stepInterval)
            {
                // advance the motor
                *stepOut |= stepBit; // set the STEP pin to 1
                dir ? pos += driveMode : pos -= driveMode;

                // keep track of how long it will take to decelerate from the current speed
                if (accelState != 0) decelDistance += (driveMode * accelState);

                // update timing vars
                lastStepTime += stepInterval;
            }
        }
        // a square wave with equal time at high and low is not necessary
        // all we need is at least 1 us step pulse (HIGH) and 1 us LOW time for Allegro chips
        // For the TI chips, the minimum required pulse time is ~2 us
        // we'll use 4 us here for a bit of padding
        else if ((curTime - lastStepTime) >= 4) *stepOut &= ~stepBit; // set the STEP pin to 0
    }
    else if (moving)
    {
        // motor has just finished moving, so reset some variables
        stop();
    }
    return moving;
}

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

bool kissStepper::moveTo(int32_t newTarget)
{
    if (!moving)
    {
        target = constrain(newTarget, reverseLimit, forwardLimit);
        bool newDir = (pos < target);
        if (newDir != dir)
        {
            digitalWrite(pinDir, (newDir ? LOW : HIGH));
            dir = newDir;
        }
        return true;
    }
    return false;
}

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

void kissStepper::decelerate(void)
{
    target = dir ? (pos + decelDistance) : (pos - decelDistance);
    target = constrain(target, reverseLimit, forwardLimit);
}

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

void kissStepper::stop(void)
{
    target = pos;
    curRP10M = 0;
    stepInterval = 4294967295UL;
    accelState = 0;
    decelDistance = 0;
    moving = false;
}

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

void kissStepper::setPos(int32_t newPos)
{
    if (!moving)
    {
        pos = constrain(newPos, reverseLimit, forwardLimit);
        target = pos;
    }
}
