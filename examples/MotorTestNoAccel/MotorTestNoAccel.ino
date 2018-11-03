/*

This software is intended for testing the kissStepper library
It measures timing and attempts to cause intentional malfunctions
Although it will not test all possible things, it is a good start
Always run this test after making changes to the library

THIS IS FOR SANITY TESTING ONLY - the results reported by this software
should not be considered a replacement for a hardware logic analyzer.

Very hacked together... consider fixing up, but low priority.

Written by Rylee Isitt
September 19, 2015

This software is licensed under the GPL v3

*/

// pinout
static const uint8_t PIN_MS1 = 5;
static const uint8_t PIN_MS2 = 6;
static const uint8_t PIN_DIR = 3;
static const uint8_t PIN_STEP = 4;
static const uint8_t PIN_ENABLE = 7;

// drive mode and steps per revolution
static const uint8_t DRIVE_MODE = 8; // drive mode (number of microsteps taken, eg 1/8th stepping = 8)
static const uint16_t REVOLUTION_FULL_STEPS = 200; // number of full steps in one revolution of the test motor (see your motor's specs/datasheet)
static const uint32_t REVOLUTION_PULSES = REVOLUTION_FULL_STEPS * DRIVE_MODE; // number of microsteps in one revolution of the test motor

#include <kissStepper.h>
// instantiate the kissStepper class for an Easy Driver
kissStepperNoAccel mot(PIN_DIR, PIN_STEP, PIN_ENABLE);

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

void resetMotor(void)
{
    delay(100);
    mot.stop();
    mot.setMaxSpeed(REVOLUTION_PULSES); // one revolution per second
    mot.prepareMove(0);
    while (mot.move() != STATE_STOPPED);
    delay(100);
}

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

void speedTest(void)
{

    // basic speed tests
    resetMotor();
    Serial.println(F("\nBasic speed test"));

    const uint8_t TESTSPEEDCOUNT = 9;
    const uint16_t testSpeeds[TESTSPEEDCOUNT] = {1,400,800,1600,3200,6400,12800,25600,65535};

    for (uint8_t i = 0; i < TESTSPEEDCOUNT; i++)
    {

        uint16_t testSpeed = testSpeeds[i];

        // move to 0
        mot.prepareMove(0);
        while (mot.move() != STATE_STOPPED);

        uint16_t testDist = testSpeed;
        mot.setMaxSpeed(testSpeed);

        mot.prepareMove(testDist);
        Serial.print(F("Testing "));
        Serial.print(String(testSpeed));
        Serial.print(F(" Hz: "));

        // forwards
        uint32_t runCount = 0;
        mot.move(); // first call of move() initializes timings
        uint32_t startTime = micros();
        while (mot.move() != STATE_STOPPED) runCount++;
        uint32_t endTime = micros();
        uint32_t time = endTime - startTime; // us

        // calculations
        float measuredSpeed = (1000000.0 * testDist) / (time);
        float runTime = (float)(time) / runCount;

        // output results
        Serial.print(String(measuredSpeed));
        Serial.print(F(" Hz, "));
        Serial.print(String(runTime));
        Serial.println(" us per iteration.");

        // back to 0
        mot.prepareMove(0);
        mot.move(); // first call of move() initializes timings
        while (mot.move() != STATE_STOPPED) runCount++;
    }
    mot.stop();
}

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

void states(void)
{

    // conduct some tests with the motor at rest and disabled

    resetMotor();
    mot.disable();
    Serial.println(F("\nMotor stationary and disabled:\n------------------------------\n"));

    if (mot.getTarget() == mot.getPos()) Serial.println(F("getTarget() == getPos() (pass)"));
    else Serial.println(F("getTarget() != getPos() (fail)"));

    delay(100);

    if (mot.getCurSpeed() == 0) Serial.println(F("getCurSpeed() == 0 (pass)"));
    else Serial.println(F("getCurSpeed() != 0 (fail)"));

    delay(100);

    if (!mot.isEnabled()) Serial.println(F("isEnabled() == FALSE (pass)"));
    else Serial.println(F("isEnabled() == TRUE (fail)"));

    delay(100);

    if (mot.getState() == STATE_STOPPED) Serial.println(F("getState() == STATE_STOPPED (pass)"));
    else Serial.println(F("getState() != STATE_STOPPED (fail)"));

    delay(100);

    // conduct some tests during and after moving the motor backwards through one revolution

    Serial.println(F("\nMotor in motion:\n-------------------------------------"));
    Serial.println(F("\nDuring movement:"));

    mot.prepareMove(REVOLUTION_PULSES);
    mot.move();
    int32_t targetDuring = mot.getTarget();
    bool forwardsDuring = mot.isMovingForwards();
    bool enabledDuring = mot.isEnabled();
    kissState_t stateDuring = mot.getState();
    while (mot.move() != STATE_STOPPED); // let motor complete motion

    // target should == REVOLUTION_PULSES
    if (targetDuring == REVOLUTION_PULSES) Serial.println(F("getTarget() == REVOLUTION_PULSES (pass)"));
    else Serial.println(F("getTarget() != REVOLUTION_PULSES (fail)"));

    delay(100);

    // forwards should == TRUE
    if (forwardsDuring) Serial.println(F("isMovingForwards() == TRUE (pass)"));
    else Serial.println(F("isMovingForwards() == FALSE (fail)"));

    delay(100);

    // should be enabled
    if (enabledDuring) Serial.println(F("isEnabled() == TRUE (pass)"));
    else Serial.println(F("isEnabled() == FALSE (fail)"));

    delay(100);

    // state should == STATE_RUN
    if (stateDuring == STATE_RUN) Serial.println(F("getState() == STATE_RUN (pass)"));
    else Serial.println(F("getState() != STATE_RUN (fail)"));

    Serial.println(F("\nAfter movement:"));

    if (mot.getPos() == REVOLUTION_PULSES) Serial.println(F("getPos() == REVOLUTION_PULSES (pass)"));
    else Serial.println(F("getPos() != REVOLUTION_PULSES (fail)"));

    delay(100);

    if (mot.getTarget() == mot.getPos()) Serial.println(F("getTarget() == getPos() (pass)"));
    else Serial.println(F("getTarget() != getPos() (fail)"));

    delay(100);

    if (mot.getCurSpeed() == 0) Serial.println(F("getCurSpeed() == 0 (pass)"));
    else Serial.println(F("getCurSpeed() != 0 (fail)"));

    delay(100);

    if (mot.isEnabled()) Serial.println(F("isEnabled() == TRUE (pass)"));
    else Serial.println(F("isEnabled() == FALSE (fail)"));

    delay(100);

    if (mot.getState() == STATE_STOPPED) Serial.println(F("getState() == STATE_STOPPED (pass)"));
    else Serial.println(F("getState() != STATE_STOPPED (fail)"));

}

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

void limits(void)
{
    resetMotor();
    Serial.println(F("\nSetting limits -1000/+1000:"));
    mot.setForwardLimit(1000);
    mot.setReverseLimit(-1000);

    mot.setMaxSpeed(200 << DRIVE_MODE);
    mot.prepareMove(10000);
    if (mot.getTarget() == mot.getForwardLimit()) Serial.println(F("moveTo() forward limit PASS"));
    else Serial.println(F("moveTo() forward limit FAIL"));
    mot.stop();

    delay(100);

    mot.prepareMove(-10000);
    if (mot.getTarget() == mot.getReverseLimit()) Serial.println(F("moveTo() reverse limit PASS"));
    else Serial.println(F("moveTo() reverse limit FAIL"));
    mot.stop();

    delay(100);

    mot.setPos(10000);
    if (mot.getPos() == mot.getForwardLimit()) Serial.println(F("setPos() forward limit PASS"));
    else Serial.println(F("setPos() forward limit FAIL"));
    mot.stop();

    delay(100);

    mot.setPos(-10000);
    if (mot.getPos() == mot.getReverseLimit()) Serial.println(F("setPos() reverse limit PASS"));
    else Serial.println(F("setPos() reverse limit FAIL"));
    mot.stop();

    mot.setForwardLimit(2147483647L);
    mot.setReverseLimit(-2147483648L);
    mot.setPos(0);

}

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

void menu(void)
{
    Serial.println("");
    Serial.println(F("Usage:"));
    Serial.println(F("<speed> tests speed accuracy and timing"));
    Serial.println(F("<states> tests class states during and after movement"));
    Serial.println(F("<limits> test position limits"));
    Serial.println(F("<menu> repeat this menu"));
    Serial.println("");
    Serial.flush();
}

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

void loop(void)
{

    // print menu
    static bool menuPrinted = false;
    if (!menuPrinted)
    {
        menu();
        menuPrinted = true;
    }

    // wait for the command
    static String command = "";
    static bool commandIn = false;
    while (Serial.available() > 0)
    {
        char c = Serial.read();
        if (c == '<') commandIn = true;
        else if (c == '>')
        {
            commandIn = false;
            command.trim();
            command.toLowerCase();
            if (command == "speed") speedTest();
            else if (command == "states") states();
            else if (command == "limits") limits();
            else if (command = "menu") menuPrinted = false;
            command = "";
        }
        else if (commandIn) command += c;
    }
}

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

void setup(void)
{

    Serial.begin(9600);

    // initialize the kissStepper
    // use 1/8th drive mode, and set the maximum speed to 200 st/s
    mot.begin();

}
