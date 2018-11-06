/*

Controls two motors.
Developed with two Easy Drivers and a Teensy 3.1.
Should be compatible with many other devices with minor changes to pinout and microstep select pins.

Written by Rylee Isitt
November 2018

This software is licensed under the GPL v3

*/

// pinout for motor controller A
static const uint8_t A_PIN_MS1 = 5;
static const uint8_t A_PIN_MS2 = 6;
static const uint8_t A_PIN_DIR = 3;
static const uint8_t A_PIN_STEP = 4;
static const uint8_t A_PIN_ENABLE = 7;

// pinout for motor controller B
static const uint8_t B_PIN_MS1 = 20;
static const uint8_t B_PIN_MS2 = 19;
static const uint8_t B_PIN_DIR = 22;
static const uint8_t B_PIN_STEP = 21;
static const uint8_t B_PIN_ENABLE = 18;

// drive mode and steps per revolution
static const uint8_t DRIVE_MODE = 8; // drive mode (number of microsteps taken, eg 1/8th stepping = 8)
static const uint16_t REVOLUTION_FULL_STEPS = 200; // number of full steps in one revolution of the test motor (see your motor's specs/datasheet)
static const uint32_t REVOLUTION_PULSES = REVOLUTION_FULL_STEPS * DRIVE_MODE; // number of microsteps in one revolution of the test motor

#include <kissStepper.h>
// instantiate the kissStepper class for an Easy Driver
kissStepper motorA(A_PIN_DIR, A_PIN_STEP, A_PIN_ENABLE);
kissStepper motorB(B_PIN_DIR, B_PIN_STEP, B_PIN_ENABLE);

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

void loop(void)
{
    motorA.prepareMove(REVOLUTION_PULSES);
    motorB.prepareMove(REVOLUTION_PULSES*-1);
    
    bool bothStopped = false;
    while (!bothStopped)
    {
        kissState_t stateA = motorA.move();
        kissState_t stateB = motorB.move();
        if ((stateA == STATE_STOPPED) && (stateB == STATE_STOPPED)) bothStopped = true;
    }
    
    motorA.prepareMove(0);
    motorB.prepareMove(0);
    
    bothStopped = false;
    while (!bothStopped)
    {
        kissState_t stateA = motorA.move();
        kissState_t stateB = motorB.move();
        if ((stateA == STATE_STOPPED) && (stateB == STATE_STOPPED)) bothStopped = true;
    }
    
}

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

void setup(void)
{
    // initialize the kissStepper classes
    motorA.begin();
    motorB.begin();

    // set drive mode pins
    // the kissStepper library does not do this for you!
    pinMode(A_PIN_MS1, OUTPUT);
    pinMode(A_PIN_MS2, OUTPUT);
    pinMode(B_PIN_MS1, OUTPUT);
    pinMode(B_PIN_MS2, OUTPUT);
    switch (DRIVE_MODE)
    {
    case 2: // half-step
        digitalWrite(A_PIN_MS1, HIGH);
        digitalWrite(A_PIN_MS2, LOW);
        digitalWrite(B_PIN_MS1, HIGH);
        digitalWrite(B_PIN_MS2, LOW);
        break;
    case 4: // quarter-step
        digitalWrite(A_PIN_MS1, LOW);
        digitalWrite(A_PIN_MS2, HIGH);
        digitalWrite(B_PIN_MS1, LOW);
        digitalWrite(B_PIN_MS2, HIGH);
        break;
    case 8: // eighth-step
        digitalWrite(A_PIN_MS1, HIGH);
        digitalWrite(A_PIN_MS2, HIGH);
        digitalWrite(B_PIN_MS1, HIGH);
        digitalWrite(B_PIN_MS2, HIGH);
        break;
    }
}
