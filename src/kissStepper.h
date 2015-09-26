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
#ifndef kissStepper_H
#define kissStepper_H

#include <Arduino.h>

enum driveMode_t: uint8_t
{
    FULL_STEP = 128,
    HALF_STEP = 64,
    MICROSTEP_4 = 32,
    MICROSTEP_8 = 16,
    MICROSTEP_16 = 8,
    MICROSTEP_32 = 4,
    MICROSTEP_64 = 2,
    MICROSTEP_128 = 1
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

    kissStepper(uint16_t motorSteps, kissPinAssignments pinAssignments, kissMicrostepConfig microstepConfig);
    ~kissStepper(void) {};
    void begin(driveMode_t mode = FULL_STEP, uint16_t maxRPM = 30, uint16_t accelRPMS = 0);
    void enable(void);
    void disable(void);
    void setDriveMode(driveMode_t mode);
    driveMode_t getDriveMode(void)
    {
        return driveMode;
    }
    void setMaxRPM(uint16_t newMaxRPM);
    uint16_t getMaxRPM(void)
    {
        return maxRP10M / 10;
    }
    void setMaxRP10M(uint16_t newMaxRP10M);
    uint16_t getMaxRP10M(void)
    {
        return maxRP10M;
    }
    uint16_t getCurRPM(void)
    {
        return curRP10M / 10;
    }
    uint16_t getCurRP10M(void)
    {
        return curRP10M;
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
    bool isMoving(void)
    {
        return moving;
    }
    bool setAccel(uint16_t RPMS);
    uint16_t getAccel(void)
    {
        return accel;
    }
    int8_t getAccelState(void)
    {
        return accelState;
    }
    int32_t forwardLimit;
    int32_t reverseLimit;
protected:
    const uint8_t pinEnable;
    const uint8_t pinDir;
    const uint8_t pinStep;
    const uint8_t pinMS1;
    const uint8_t pinMS2;
    const uint8_t pinMS3;
    const uint8_t MS1Config;
    const uint8_t MS2Config;
    const uint8_t MS3Config;
    const uint16_t motorStPerRev;
    const driveMode_t maxMicrostepMode;
    uint8_t stepBit;
    volatile uint8_t *stepOut;
    int32_t pos;
    int32_t target;
    uint16_t maxRP10M;
    uint16_t curRP10M;
    driveMode_t driveMode;
    uint32_t stepInterval;
    uint32_t accelInterval;
    bool enabled;
    bool moving;
    bool dir;
    int8_t accelState;
    uint32_t accelDistance;
    uint32_t lastAccelTime;
    uint32_t lastStepTime;
    uint16_t accel;
    void setCurRP10M(uint16_t newCurRP10M);
};

#endif
