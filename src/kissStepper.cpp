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
// The kissStepper constructor
// ----------------------------------------------------------------------------------------------------

kissStepper::kissStepper(kissPinAssignments pinAssignments, kissMicrostepConfig microstepConfig)
    :pinDir(pinAssignments.pinDir), pinStep(pinAssignments.pinStep), pinEnable(pinAssignments.pinEnable),
     pinMS1(pinAssignments.pinMS1), pinMS2(pinAssignments.pinMS2), pinMS3(pinAssignments.pinMS3),
     maxMicrostepMode(microstepConfig.maxMicrostepMode), MS1Config(microstepConfig.MS1Config), MS2Config(microstepConfig.MS2Config), MS3Config(microstepConfig.MS3Config),
	 fullStepVal(1 << microstepConfig.maxMicrostepMode) {}

// ----------------------------------------------------------------------------------------------------
// Initialize the motor in a default state:
// - Disabled (ENABLE pin high)
// - Set to move forwards (DIR pin low)
// - STEP pin low
// - Default to full step drive mode if none specified by the user
// - Default to 30 RPM if speed not specified by the users
// - Default to no acceleration
// ----------------------------------------------------------------------------------------------------

void kissStepper::begin(driveMode_t mode, uint16_t maxStepsPerSec, uint16_t accelStepsPerSecPerSec)
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
    pos = 0;
	speedAdjustCounter = 0;
    stop();
    forwardLimit = 2147483647L;
    reverseLimit = -2147483648L;
    setDriveMode(mode);
    setMaxSpeed(maxStepsPerSec);
    setAccel(accelStepsPerSecPerSec);

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
    if ((uint8_t)mode > (uint8_t)maxMicrostepMode) mode = maxMicrostepMode;
	stepVal = fullStepVal >> mode;
	
    // set the pin states
	uint8_t bitMask = 128 >> mode;
    if (pinMS1 < 255) digitalWrite(pinMS1, ((MS1Config & bitMask) ? HIGH : LOW));
    if (pinMS2 < 255) digitalWrite(pinMS2, ((MS2Config & bitMask) ? HIGH : LOW));
    if (pinMS3 < 255) digitalWrite(pinMS3, ((MS3Config & bitMask) ? HIGH : LOW));

    driveMode = mode;
    setCurSpeed(curSpeed);
}

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

void kissStepper::setMaxSpeed(uint16_t stepsPerSec)
{
    maxSpeed = stepsPerSec;
    // if the motor is moving and acceleration is off, change speed immediately
    if ((!accel) && (moveState != 0)) setCurSpeed(stepsPerSec);
}

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

void kissStepper::setCurSpeed(uint16_t stepsPerSec)
{
    // The 1000000 "magic number" is the number of microseconds in 1 second
    if (stepsPerSec > 0)
    {	
		uint32_t numer = 500000UL;
		uint32_t denom = (uint32_t)stepsPerSec << driveMode;
		stepInterval = numer / denom;
		speedAdjustProbability = ((numer % denom) * 255) / denom;
    }
    else stepInterval = 4294967295UL;
    curSpeed = stepsPerSec;
}

// ----------------------------------------------------------------------------------------------------
// This method sets the motor acceleration in RPM/s
// ----------------------------------------------------------------------------------------------------

bool kissStepper::setAccel(uint16_t stepsPerSecPerSec)
{
    // calculate the time interval at which to increment curSpeed
    // and recalculate decelDistance
    // but only allow if not currently accelerating
    if (accelState == 0)
    {
        if (stepsPerSecPerSec > 0)
        {
            accelInterval = 1000000UL / stepsPerSecPerSec;
			if (((1000000UL % stepsPerSecPerSec) << 1) >= stepsPerSecPerSec) accelInterval++;
            calcDecel();
        }
        accel = stepsPerSecPerSec;
		calcDecel();
        return true;
    }
    return false;
}

// ----------------------------------------------------------------------------------------------------
// This method figures out the distance required to decelerate from the current speed
// ----------------------------------------------------------------------------------------------------

void kissStepper::calcDecel(void)
{
	if (accel != 0)
	{
		decelDistance = ((uint32_t)curSpeed * curSpeed * fullStepVal) / ((uint32_t)accel << 1);	
	}
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
    if (moveState != 0)
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
			uint32_t stepsRemaining = (target > pos) ? (target - pos) : (pos - target);
			bool mustDecel = stepsRemaining < decelDistance;
            if (!mustDecel && (curSpeed < maxSpeed))   // accelerate
            {
                if (accelState != 1)
                {
                    accelState = 1;
                    lastAccelTime = curTime;
                }
            }
            else if ((mustDecel && (curSpeed > 1)) || (curSpeed > maxSpeed))     // decelerate
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
                setCurSpeed(curSpeed+accelState);
                lastAccelTime += accelInterval;
				calcDecel();
            }
        }

        // Step, if it's time...
        // Adding stepInterval to lastStepTime produces more accurate timing than setting lastStepTime = curTime
		if ((curTime - lastStepTime) >= stepInterval)
		{
			if (!(*stepOut & stepBit))
			{
				// check if the target is reached
				int32_t newPos = (moveState == 1) ? (pos + stepVal) : (pos - stepVal);
				if (((moveState == 1) && (newPos <= target)) || ((moveState == -1) && (newPos >= target)))
				{
					// the target is not yet reached, so advance the motor
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
			if (speedAdjustCounter++ < speedAdjustProbability) lastStepTime++;
			
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
    if (moveState == 0)
    {
		// enable the motor controller if necessary
		if (!enabled) enable();
		
		// constrain the target between reverseLimit and forwardLimit
		target = constrain(newTarget, reverseLimit, forwardLimit);
		
		// set moveState
		if (target == pos)
			moveState = 0;
		else if (target > pos)
			moveState = 1;
		else
			moveState = -1;
		
		if (moveState != 0)
		{
			// set the DIR pin
			digitalWrite(pinDir, ((moveState == 1) ? LOW : HIGH));
			
			// reset timing
			lastStepTime = micros();
			lastAccelTime = lastStepTime;
			
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
	target = ((moveState == 1) ? (pos + decelDistance) : (pos - decelDistance));
	target = constrain(target, reverseLimit, forwardLimit);
}

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

void kissStepper::stop(void)
{
    target = pos;
    curSpeed = 0;
    stepInterval = 4294967295UL;
    accelState = 0;
    decelDistance = 0;
	moveState = 0;
}

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

void kissStepper::setPos(int32_t newPos)
{
    if (moveState == 0)
    {
        pos = constrain(newPos, reverseLimit, forwardLimit);
        target = pos;
    }
}
