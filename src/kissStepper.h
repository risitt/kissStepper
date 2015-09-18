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
#ifndef kissStepper_H
#define kissStepper_H

#include <Arduino.h>

class kissStepper
{
public:

	kissStepper(uint8_t pinEnable, uint8_t pinDir, uint8_t pinStep);
	kissStepper(uint8_t pinEnable, uint8_t pinDir, uint8_t pinStep, uint8_t pinMS1);
	kissStepper(uint8_t pinEnable, uint8_t pinDir, uint8_t pinStep, uint8_t pinMS1, uint8_t pinMS2);
	kissStepper(uint8_t pinEnable, uint8_t pinDir, uint8_t pinStep, uint8_t pinMS1, uint8_t pinMS2, uint8_t pinMS3);
    ~kissStepper(void) {};
	enum driveMode_t
	{
		FULL = 1,
		HALF = 2,
		QUARTER = 4,
		EIGHTH = 8,
		SIXTEENTH = 16
	};
	void begin(uint16_t motorSteps, driveMode_t mode = FULL, uint16_t maxRPM = 30, uint16_t accel = 0);
    void enable(void);
    void disable(void);
	void setDriveMode(driveMode_t mode);
	driveMode_t getDriveMode(void) { return driveMode; }
	void setMaxRPM(uint16_t maxRPM) { maxRP10M = maxRPM * 10; }
	uint16_t getMaxRPM(void) { return maxRP10M / 10; }
	uint16_t getCurRPM(void) { return curRP10M / 10; }
    bool work(void);
    bool moveTo(int32_t newTarget);
	bool moveForward(void);
	bool moveBackward(void);
	void stop(void) { target = dir ? pos + accelDistance : pos - accelDistance; }
	void hardStop(void) { target = pos; }
	int32_t getPos(void) { return pos; }
	int32_t getTarget(void) { return target; }
    bool isEnabled(void) { return enabled; }
	bool setAccel(uint16_t revPerMinPerMin);
	uint16_t getAccel(void) { return accel; }
private:
    const uint8_t pinEnable = 255;
    const uint8_t pinDir;
    const uint8_t pinStep;
    const uint8_t pinMS1;
    const uint8_t pinMS2;
	const uint8_t pinMS3;
	const uint8_t fullStepSize = 16;
	uint16_t motorStPerRev;
	int32_t pos;
	int32_t target;
	uint16_t maxRP10M;
	uint16_t curRP10M;
	driveMode_t driveMode;
	uint8_t stepSize;
	uint32_t stepInterval;
	uint32_t accelInterval;
	bool enabled;
	bool dir;
	bool stepOn;
	uint32_t lastAccelTime;
	uint32_t lastStepTime;
	uint16_t accel;
	uint32_t accelDistance;
	void setCurRP10M(uint16_t newSpeed);
};

#endif
