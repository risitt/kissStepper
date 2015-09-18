/*
Keep it Simple Stepper - a lightweight library for the Easy Driver, Big Easy Driver, and Allegro stepper motor drivers that use a Step/Dir interface
Written by Rylee Isitt. September 14, 2015
License: GNU Lesser General Public License (LGPL) V2.1

Despite the existence of several excellent libraries for driving stepper motors, I created this one to fullfill the following needs:
* LGPL instead of GPL, so that you can use it in your own project with few licensing restrictions (please read the LGPL V2.1 for details).
* Low memory and processing demands
* Consistent motor speed regardless of drive mode
* Consistent position index (measured in 1/16th steps), so even after changing drive modes, position 1000 will refer to the same location as before
* Automatic handling of MS1, MS2, and MS3 (microstep select) pins if desired
* Automatic handling of Enable pin if desired (set to 255 if you don't want to use this feature)
* Acceleration for driving heavier loads and reaching higher speeds before the motor stalls
*/
#include "kissStepper.h"

kissStepper::kissStepper(uint8_t pinEnable, uint8_t pinDir, uint8_t pinStep)
	: pinEnable(pinEnable), pinDir(pinDir), pinStep(pinStep), pinMS1(255), pinMS2(255), pinMS3(255) {}

kissStepper::kissStepper(uint8_t pinEnable, uint8_t pinDir, uint8_t pinStep, uint8_t pinMS1)
	: pinEnable(pinEnable), pinDir(pinDir), pinStep(pinStep), pinMS1(pinMS1), pinMS2(255), pinMS3(255) {}

kissStepper::kissStepper(uint8_t pinEnable, uint8_t pinDir, uint8_t pinStep, uint8_t pinMS1, uint8_t pinMS2)
	: pinEnable(pinEnable), pinDir(pinDir), pinStep(pinStep), pinMS1(pinMS1), pinMS2(pinMS2), pinMS3(255) {}

kissStepper::kissStepper(uint8_t pinEnable, uint8_t pinDir, uint8_t pinStep, uint8_t pinMS1, uint8_t pinMS2, uint8_t pinMS3)
	: pinEnable(pinEnable), pinDir(pinDir), pinStep(pinStep), pinMS1(pinMS1), pinMS2(pinMS2), pinMS3(pinMS3) {}

// ----------------------------------------------------------------------------------------------------
// Initialize the motor in a default state:
// - Disabled (ENABLE pin high)
// - Set to move forwards (DIR pin low)
// - STEP pin low
// - Default to full step drive mode if none specified by the user
// - Default to 30 RPM if speed not specified by the user
// - Default to no acceleration
// ----------------------------------------------------------------------------------------------------

void kissStepper::begin(uint16_t motorSteps, driveMode_t mode, uint16_t maxRPM, uint16_t accel)
{
	// set pins to output
	pinMode(pinDir, OUTPUT);
	pinMode(pinStep, OUTPUT);
	if (pinMS1 != 255) pinMode(pinMS1, OUTPUT);
	if (pinMS2 != 255) pinMode(pinMS2, OUTPUT);
	if (pinMS3 != 255) pinMode(pinMS3, OUTPUT);

	// enable pin
	if (pinEnable < 255)
	{
		pinMode(pinEnable, OUTPUT);
		digitalWrite(pinEnable, HIGH);
	}

	// defaults
	digitalWrite(pinDir, LOW); // forwards
	digitalWrite(pinStep, LOW);
	motorStPerRev = motorSteps;
	dir = true;
	stepOn = false;
	curRP10M = accel = accelInterval = pos = target = 0;
	setDriveMode(mode);
	setMaxRPM(maxRPM);
	setAccel(accel);
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
	enabled = false;
}

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

void kissStepper::setDriveMode(driveMode_t mode)
{
	switch (mode) {
	case SIXTEENTH:
		// sixteenth stepping requires at least three MS lines
		if (pinMS3 < 255)
		{
			digitalWrite(pinMS1, HIGH);
			digitalWrite(pinMS2, HIGH);
			digitalWrite(pinMS3, HIGH);
		}
		break;
	case EIGHTH:
		// eighth stepping requires at least two MS lines
		if (pinMS2 < 255)
		{
			digitalWrite(pinMS1, HIGH);
			digitalWrite(pinMS2, HIGH);
		}
		if (pinMS3 < 255) digitalWrite(pinMS3, LOW);
		break;
	case QUARTER:
		// quarter stepping requires at least two MS lines
		if (pinMS2 < 255)
		{
			digitalWrite(pinMS1, LOW);
			digitalWrite(pinMS2, HIGH);
		}
		if (pinMS3 < 255) digitalWrite(pinMS3, LOW);
		break;
	case HALF:
		// half stepping requires at least one MS line
		if (pinMS1 < 255) digitalWrite(pinMS1, HIGH);
		if (pinMS2 < 255) digitalWrite(pinMS2, LOW);
		if (pinMS3 < 255) digitalWrite(pinMS3, LOW);
		break;
	case FULL:
	default:
		if (pinMS1 < 255) digitalWrite(pinMS1, LOW);
		if (pinMS2 < 255) digitalWrite(pinMS2, LOW);
		if (pinMS3 < 255) digitalWrite(pinMS3, LOW);
		break;
	}
	driveMode = mode;

	// keep this calculation out of the switch statement and don't check for presence of pins
	// this is so that even if MS lines are not controlled with the MCU, the user can still inform	
	// this library about the chosen stepping mode for accurate speed calculations
	stepSize = fullStepSize / mode;
	setCurRP10M(curRP10M);
}

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

void kissStepper::setCurRP10M(uint16_t RP10M)
{
	// stepInterval is half the period / twice the frequency of steps
	// because it toggles the step pin both on and off in a square wave
	// with equal time at low and high.
	// The 300000000 "magic number" is the number of microseconds in 300 seconds.
	// 300 seconds is used because the calculation is based on revolutions per 10 minutes (600 seconds)
	// and stepInterval needs to be half the period of the square wave.
	if ((RP10M > 0) && (motorStPerRev > 0))
		stepInterval = ((300000000UL / fullStepSize) * stepSize) / ((uint32_t)RP10M * motorStPerRev);
	else
		stepInterval = -1;
	curRP10M = RP10M;
}

// ----------------------------------------------------------------------------------------------------
// This method sets the motor acceleration in revolutions per minute per minute
// ----------------------------------------------------------------------------------------------------

bool kissStepper::setAccel(uint16_t RPMPM)
{
	// calculate the time interval at which to increment curRP10M
	// and recalculate accelDistance
	// but only allow if not currently accelerating
	if ((curRP10M == 0) || (curRP10M == maxRP10M)) {
		if (RPMPM > 0) {
			accelInterval = 6000000UL / RPMPM;
			accelDistance = (accelDistance*accel) / RPMPM;
		}
		else
			accelInterval = 0;
				
		accel = RPMPM;
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

	static bool isMoving = false;

	// check if it's necessary to move the motor
	// compare to stepSize to prevent the motor from twitching back and forth around the target position
	uint32_t stepsRemaining = abs(target - pos);
	if (stepsRemaining >= stepSize)
	{
		uint32_t curTime = micros();
		
		// reset timing
		// this block must always run once, and only once, at the start of a step sequencenoe
		if (!isMoving) {
			lastStepTime = curTime; // this prevents lastStepTime from lagging behind
			lastAccelTime = curTime; // this prevents lastAccelTime from lagging behind
			accelDistance = 0;
			isMoving = true;
		}
		
		// Handle speed adjustments
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
		if (accel == 0)
		{
			// if not using acceleration, change speed instantaneously
			if (curRP10M != maxRP10M) setCurRP10M(maxRP10M);
		}
		else
		{
			if ((stepsRemaining > accelDistance) && (curRP10M < maxRP10M))   // accelerate
			{
				if ((curTime - lastAccelTime) >= accelInterval)
				{
					setCurRP10M(curRP10M+1);
					lastAccelTime += accelInterval;
				}
			}
			else if (((stepsRemaining < accelDistance) && (curRP10M > 1)) || (curRP10M > maxRP10M))     // decelerate
			{
				if ((curTime - lastAccelTime) >= accelInterval)
				{
					setCurRP10M(curRP10M-1);
					lastAccelTime += accelInterval;
				}
			}
			else
			{
				// need to do this to prevent lastAccelTime from lagging far behind
				// and creating timing problems
				lastAccelTime = curTime;
			}
		}
		
		// Step, if it's time...
		// Adding stepInterval to lastStepTime produces more accurate timing than setting lastStepTime = curTime
		if ((curTime - lastStepTime) >= stepInterval)
		{
			// enable if needed
			if ((!enabled) && (pinEnable < 255)) enable();
					
			// advance the motor
			if (!stepOn) {
				digitalWrite(pinStep, HIGH);
				dir ? pos += stepSize : pos -= stepSize;
				if ((accel > 0) && (curRP10M < maxRP10M)) accelDistance += stepSize;
				else if ((accel > 0) && (curRP10M > maxRP10M)) accelDistance -= stepSize;
			}
			else {
				digitalWrite(pinStep, LOW);
			}
			stepOn = !stepOn;

			// update timing vars
			lastStepTime += stepInterval;
		}
		
	}
	else if (isMoving)
	{
		if (curRP10M > 0) setCurRP10M(0);
		isMoving = false; // motor is not moving
	}
	return isMoving;
}

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

bool kissStepper::moveTo(int32_t newTarget)
{
	if (curRP10M == 0) {
		target = newTarget;
		bool newDir = (pos < target);
		if ((newDir) && (!dir))
		{
			digitalWrite(pinDir, LOW);
			dir = true;
		}
		else if ((!newDir) && (dir))
		{
			digitalWrite(pinDir, HIGH);
			dir = false;
		}
		return true;
	}
	return false;
}

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

bool kissStepper::moveForward(void)
{
	if (curRP10M == 0) {
		target = 2147483647L;
		if (!dir) {
			digitalWrite(pinDir, LOW);
			dir = true;
		}
		return true;
	}
	return false;
}

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

bool kissStepper::moveBackward(void)
{
	if (curRP10M == 0) {
		target = -2147483648L;
		if (dir) {
			digitalWrite(pinDir, HIGH);
			dir = false;
		}
		return true;
	}
	return false;
}
