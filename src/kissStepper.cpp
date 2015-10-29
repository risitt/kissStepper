/*
Keep it Simple Stepper - a lightweight library for the Easy Driver, Big Easy Driver, Allegro stepper motor drivers and others that use a Step/Dir interface
Written by Rylee Isitt. September 21, 2015
License: GNU Lesser General Public License (LGPL) V2.1

Despite the existence of several excellent libraries for driving stepper motors, I created this one to fullfill the following needs:
* LGPL instead of GPL, so that you can use it in your own project with few licensing restrictions (please read the LGPL V2.1 for details).
* Low memory and processing demands
* Consistent motor speed regardless of drive mode
* Consistent position index for a given value of maxMicrostepMode, regardless of the current drive mode
* Automatic handling of MS1, MS2, and MS3 (microstep select) pins if desired
* Automatic handling of Enable pin if desired (set to 255 if you don't want to use this feature)
* Acceleration for driving heavier loads and reaching higher speeds before the motor stalls
*/
#include "kissStepper.h"

// ----------------------------------------------------------------------------------------------------
// Initialize the motor in a default state:
// - Disabled (ENABLE pin high)
// - Set to move forwards (DIR pin low)
// - STEP pin low
// - Default to highest resolution microstep mode if none specified by the user
// - Default to 100 steps/sec if speed not specified by the users
// - Default to no acceleration
// ----------------------------------------------------------------------------------------------------

void kissStepper::begin(driveMode_t mode, uint16_t maxStepsPerSec, uint16_t accelStepsPerSecPerSec)
{
    // set pins to output
    if (pinEnable != pinNotSet) pinMode(pinEnable, OUTPUT);
    pinMode(pinDir, OUTPUT);
    pinMode(pinStep, OUTPUT);
    if (pinMS1 != pinNotSet) pinMode(pinMS1, OUTPUT);
    if (pinMS2 != pinNotSet) pinMode(pinMS2, OUTPUT);
    if (pinMS3 != pinNotSet) pinMode(pinMS3, OUTPUT);

    // initial position index and limits
    pos = 0;
    forwardLimit = defaultForwardLimit;
    reverseLimit = defaultReverseLimit;

    // initial pin states
    disable();
    stop();
    digitalWrite(pinDir, PINVAL_FORWARD);
    digitalWrite(pinStep, LOW);

    // defaults
    setDriveMode(mode);
    setMaxSpeed(maxStepsPerSec);
    setAccel(accelStepsPerSecPerSec);
    correctionCounter = 0;

    // this allows us to convert from a standard Arduino pin number to an AVR port
    // for faster digital writes in the work() method at the cost of a few bytes of memory
    // we don't use this technique for other digitalWrites because they are infrequent
    stepBit = digitalPinToBitMask(pinStep);
    stepOut = portOutputRegister(digitalPinToPort(pinStep));

}

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

void kissStepper::enable(void)
{
    if (pinEnable != pinNotSet) digitalWrite(pinEnable, PINVAL_ENABLED);
    enabled = true;
}

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

void kissStepper::disable(void)
{
    if (pinEnable != pinNotSet)
    {
        delay(50); // this short delay stops motor momentum
        digitalWrite(pinEnable, PINVAL_DISABLED);
    }
    target = pos;
    enabled = false;
}

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

void kissStepper::setDriveMode(driveMode_t mode)
{
    // do not allow modes beyond the limit set by the user
    if ((uint8_t)mode > (uint8_t)maxMicrostepMode) mode = maxMicrostepMode;
    stepVal = fullStepVal >> mode;

    // set the pin states
    uint8_t bitMask = 128 >> mode;
    if (pinMS1 != pinNotSet) digitalWrite(pinMS1, ((MS1Config & bitMask) ? HIGH : LOW));
    if (pinMS2 != pinNotSet) digitalWrite(pinMS2, ((MS2Config & bitMask) ? HIGH : LOW));
    if (pinMS3 != pinNotSet) digitalWrite(pinMS3, ((MS3Config & bitMask) ? HIGH : LOW));

    driveMode = mode;
    setCurSpeed(curSpeed);
}

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

void kissStepper::setMaxSpeed(uint16_t stepsPerSec)
{
    maxSpeed = stepsPerSec;
    // if the motor is moving and acceleration is off, change speed immediately
    if ((!accel) && (moveState != STOPPED)) setCurSpeed(stepsPerSec);
}

// ----------------------------------------------------------------------------------------------------
// The errorCorrection variable is used in work() to correct the rounding error that occurs
// in the calculation of stepInterval. The errorCorrection variable indicates the number of times
// out of 255 that a timing correction needs to be applied. Its value is compared to a counter,
// correctionCounter, in order to determine whether or not to apply a correction.
// In effect, this adds 8 bits worth of "decimal places" to stepInterval.
// ----------------------------------------------------------------------------------------------------

void kissStepper::setCurSpeed(uint16_t stepsPerSec)
{
    if (stepsPerSec > 0)
    {
        uint32_t uStepsPerSec = (uint32_t)stepsPerSec << driveMode;
        stepInterval = halfSecond / uStepsPerSec;
        errorCorrection = ((halfSecond % uStepsPerSec) << 8) / uStepsPerSec;
        if (curSpeed == 0) lastStepTime = micros() - stepInterval;
    }
    else stepInterval = maxTimeInterval;
    curSpeed = stepsPerSec;
}

// ----------------------------------------------------------------------------------------------------
// This method sets the motor acceleration in RPM/s
// ----------------------------------------------------------------------------------------------------

void kissStepper::setAccel(uint16_t stepsPerSecPerSec)
{
    // calculate the time interval at which to increment curSpeed
    // and recalculate decelDistance
    if (stepsPerSecPerSec > 0)
        accelInterval = (oneSecond + (stepsPerSecPerSec >> 1)) / stepsPerSecPerSec;
    accel = stepsPerSecPerSec;
    calcDecel();
}

// ----------------------------------------------------------------------------------------------------
// This method figures out the distance required to decelerate from the current speed
// ----------------------------------------------------------------------------------------------------

void kissStepper::calcDecel(void)
{
    if (accel != 0)
        decelDistance = ((uint32_t)curSpeed * curSpeed * fullStepVal) / ((uint32_t)accel << 1);
    else
        decelDistance = 0;
}

// ----------------------------------------------------------------------------------------------------
// Makes the motor work. Call repeatedly and often for smooth motion.
// Returns true if the motor is moving, otherwise false.
// ----------------------------------------------------------------------------------------------------
bool kissStepper::work(void)
{

    // check if it's necessary to move the motor
    if (moveState != STOPPED)
    {

        uint32_t curTime = micros();

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
            accelState_t newAccelState;
            uint32_t distRemaining = ((moveState == FORWARD) ? (target - pos) : (pos - target));
            if (distRemaining > decelDistance)
                newAccelState = ((curSpeed == maxSpeed) ? CONSTVEL : ((curSpeed < maxSpeed) ? ACCELERATING : DECELERATING));
            else
                newAccelState = (curSpeed > 1) ? DECELERATING : CONSTVEL;

            if (newAccelState != CONSTVEL)
            {
                if (accelState != newAccelState)
                    lastAccelTime = curTime;

                if ((curTime - lastAccelTime) >= accelInterval)
                {
                    setCurSpeed(curSpeed+newAccelState);
                    lastAccelTime += accelInterval;
                    calcDecel();
                }
            }
            accelState = newAccelState;
        }

        // Step, if it's time...
        // Adding stepInterval to lastStepTime produces more accurate timing than setting lastStepTime = curTime
        if ((curTime - lastStepTime) >= stepInterval)
        {
            if (!(*stepOut & stepBit))
            {
                // check if the target is reached
                int32_t newPos = (moveState == FORWARD) ? (pos + stepVal) : (pos - stepVal);
                if (((moveState == FORWARD) && (newPos <= target)) || ((moveState == BACKWARD) && (newPos >= target)))
                {
                    // the target is not yet reached, so advance the position index
                    pos = newPos;
                }
                else
                {
                    // the target is reached
                    stop();
                    return false;
                }
            }

            // toggle the step pin
            *stepOut ^= stepBit;

            // increment lastStepTime
            lastStepTime += stepInterval;

            // this adds a correction to the timing
            if (correctionCounter < errorCorrection) lastStepTime++;
            correctionCounter += counterIncrement;
        }

        return true;
    }
    return false;
}

// ----------------------------------------------------------------------------------------------------
// This method starts the motor moving towards a target
// ----------------------------------------------------------------------------------------------------

bool kissStepper::moveTo(int32_t newTarget)
{
    if (moveState == STOPPED)
    {
        // enable the motor controller if necessary
        if (!enabled) enable();

        // constrain the target between reverseLimit and forwardLimit
        target = constrain(newTarget, reverseLimit, forwardLimit);

        // set moveState
        moveState = (target > pos) ? FORWARD : ((target < pos) ? BACKWARD : STOPPED);

        if (moveState != STOPPED)
        {
            // set the DIR pin
            digitalWrite(pinDir, ((moveState == FORWARD) ? PINVAL_FORWARD : PINVAL_BACKWARD));

            // if not accelerating, start motor at full speed
            if (!accel) setCurSpeed(maxSpeed);

            return true;
        }
    }
    return false;
}

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

void kissStepper::decelerate(void)
{
    target = ((moveState == FORWARD) ? (pos + decelDistance) : (pos - decelDistance));
    target = constrain(target, reverseLimit, forwardLimit);
}

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

void kissStepper::stop(void)
{
    target = pos;
    curSpeed = 0;
    stepInterval = maxTimeInterval;
    accelState = CONSTVEL;
    decelDistance = 0;
    moveState = STOPPED;
}

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

void kissStepper::setPos(int32_t newPos)
{
    if (moveState == STOPPED)
    {
        pos = constrain(newPos, reverseLimit, forwardLimit);
        target = pos;
    }
}
