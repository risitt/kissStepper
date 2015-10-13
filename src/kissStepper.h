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
#ifndef kissStepper_H
#define kissStepper_H

#include <Arduino.h>

enum driveMode_t: uint8_t
{
    FULL_STEP = 0,
    HALF_STEP = 1,
    MICROSTEP_4 = 2,
    MICROSTEP_8 = 3,
    MICROSTEP_16 = 4,
    MICROSTEP_32 = 5,
    MICROSTEP_64 = 6,
    MICROSTEP_128 = 7
};

struct kissPinAssignments
{
    kissPinAssignments(uint8_t pinDir, uint8_t pinStep, uint8_t pinEnable = 255, uint8_t pinMS1 = 255, uint8_t pinMS2 = 255, uint8_t pinMS3 = 255)
        : pinDir(pinDir), pinStep(pinStep), pinEnable(pinEnable), pinMS1(pinMS1), pinMS2(pinMS2), pinMS3(pinMS3) {}

    const uint8_t pinEnable;
    const uint8_t pinDir;
    const uint8_t pinStep;
    const uint8_t pinMS1;
    const uint8_t pinMS2;
    const uint8_t pinMS3;
};

struct kissMicrostepConfig
{
    kissMicrostepConfig(driveMode_t maxMicrostepMode, uint8_t MS1Config = 88, uint8_t MS2Config = 56, uint8_t MS3Config = 8)
        : maxMicrostepMode(maxMicrostepMode), MS1Config(MS1Config), MS2Config(MS2Config), MS3Config(MS3Config) {}

    const driveMode_t maxMicrostepMode;
    const uint8_t MS1Config;
    const uint8_t MS2Config;
    const uint8_t MS3Config;
};

class kissStepper
{
public:

    kissStepper(kissPinAssignments pinAssignments, kissMicrostepConfig microstepConfig);
    ~kissStepper(void) {};
    void begin(driveMode_t mode = FULL_STEP, uint16_t maxStepsPerSec = 100, uint16_t accelStepsPerSecPerSec = 0);
    void enable(void);
    void disable(void);
    void setDriveMode(driveMode_t mode);
    driveMode_t getDriveMode(void)
    {
        return driveMode;
    }
    void setMaxSpeed(uint16_t stepsPerSec);
    uint16_t getMaxSpeed(void)
    {
        return maxSpeed;
    }
    uint16_t getCurSpeed(void)
    {
        return curSpeed;
    }
    bool work(void);
    bool moveTo(int32_t newTarget);
    void decelerate(void);
    void stop(void);
    void setPos(int32_t newPos);
    int32_t getPos(void)
    {
        return pos;
    }
    int32_t getTarget(void)
    {
        return target;
    }
    bool isEnabled(void)
    {
        return enabled;
    }
    void setAccel(uint16_t stepsPerSecPerSec);
    uint16_t getAccel(void)
    {
        return accel;
    }
    int8_t getAccelState(void)
    {
        return accelState;
    }
    int8_t getMoveState(void)
    {
        return moveState;
    }
    int32_t forwardLimit;
    int32_t reverseLimit;
    const uint8_t fullStepVal;
private:
	static const uint32_t halfSecond = 500000UL;
	static const uint32_t oneSecond = 1000000UL;
	static const uint8_t counterIncrement = 17;
	static const uint8_t pinNotSet = 255;
	static const int32_t defaultForwardLimit = 2147483647L;
	static const int32_t defaultReverseLimit = -2147483648L;
	static const uint32_t maxTimeInterval = 4294967295UL;
    const uint8_t pinEnable;
    const uint8_t pinDir;
    const uint8_t pinStep;
    const uint8_t pinMS1;
    const uint8_t pinMS2;
    const uint8_t pinMS3;
    const uint8_t MS1Config;
    const uint8_t MS2Config;
    const uint8_t MS3Config;
    const driveMode_t maxMicrostepMode;
    uint8_t stepBit;
    volatile uint8_t *stepOut;
    int32_t pos;
    int32_t target;
    uint16_t maxSpeed;
    uint16_t curSpeed;
    uint8_t errorCorrection;
    uint8_t correctionCounter;
    driveMode_t driveMode;
    uint8_t stepVal;
    uint32_t stepInterval;
    uint32_t accelInterval;
    bool enabled;
    int8_t accelState;
    int8_t moveState;
    uint32_t decelDistance;
    uint32_t lastAccelTime;
    uint32_t lastStepTime;
    uint16_t accel;
    void setCurSpeed(uint16_t stepsPerSec);
    void calcDecel(void);
};

#endif
