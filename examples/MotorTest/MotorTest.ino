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
kissStepper mot(PIN_DIR, PIN_STEP, PIN_ENABLE);

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

void resetMotor(void)
{
    delay(100);
    mot.stop();
    mot.setMaxSpeed(REVOLUTION_PULSES); // one revolution per second
    mot.setAccel(0);
    mot.prepareMove(0);
    while (mot.move() != STATE_STOPPED);
    delay(100);
}

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

void speedProfiles(void)
{

    bool failed = false;
    Serial.println(F("Testing generated speed profiles...\n"));

    uint16_t speed, accel, dist, topSpeed;
    int16_t delta;
    uint32_t totalDistA, accelDist, runDist, decelDist, totalDistB, maxAccelDist, accDecDiff;
    int32_t prevPos = mot.getPos();
    uint32_t lastTime = millis();
    uint16_t updateInterval = 500;
    uint8_t updateMultiple = 20;
    uint8_t updateCounter = 1;
    float accelTime, runTime, decelTime;

    // set random seed
    randomSeed(analogRead(0));
    
    uint32_t i=0;
    while(true)
    {
        i++;
        // set random speed, accel, and delta
        speed = random(1,65535);
        accel = random(1,65535);
        delta = random(-32767,32767);
        dist = (delta > 0) ? delta : -delta;
        
        // prepare the movement
        mot.stop();
        mot.setMaxSpeed(speed);
        mot.setAccel(accel);
        mot.setPos(0);
        mot.prepareMove(delta);
        
        topSpeed = mot.getTopSpeed();
        maxAccelDist = mot.calcMaxAccelDist();
        totalDistA = mot.getDistRemaining();
        accelDist = mot.getAccelDist();
        runDist = mot.getRunDist();
        decelDist = mot.getDecelDist();
        totalDistB = accelDist + runDist + decelDist;
        accDecDiff = (accelDist > decelDist) ? (accelDist - decelDist) : (decelDist - accelDist);

        accelTime = accelDist / (topSpeed / 2.0);
        decelTime = decelDist / (topSpeed / 2.0);
        runTime = runDist / (float)topSpeed;
        
        if (totalDistA != totalDistB)
        {
            Serial.println(F("\n\ngetDistRemaining() != accelDist+runDist+decelDist"));
            failed = true;
        }

        if (totalDistA != dist)
        {
            Serial.println(F("\n\ngetDistRemaining() != distance to target"));
            failed = true;
        }

        if ((accel == 0) && (accelDist != 0))
        {
            Serial.println(F("\n\nUnexpected acceleration distance"));
            failed = true;
        }

        if ((accel == 0) && (decelDist != 0))
        {
            Serial.println(F("\n\nUnexpected deceleration distance"));
            failed = true;
        }

        if (accDecDiff > 1)
        {
            Serial.println(F("\n\nDifference between acceleration and deceleration distance is too large"));
            failed = true;
        }

        if ((maxAccelDist > 0) && (dist > 0) && ((accelDist + decelDist) == 0))
        {
            Serial.println(F("\n\nUnexpected lack of acceleration or deceleration"));
            failed = true;
        }

        if ((maxAccelDist*2 > dist) && (runDist > 0))
        {
            Serial.println(F("\n\nUnexpected positive run distance (expected triangular speed profile)"));
            failed = true;
        }

        if ((runDist > 0) && ((accelDist + decelDist) != maxAccelDist*2))
        {
            Serial.println(F("\n\nUnexpectedly large run distance, small accel+decel distance"));
            failed = true;
        }

        uint32_t curTime = millis();
        if ((curTime - lastTime >= updateInterval) || (failed))
        {
            
            Serial.print(F("-"));
            
            if (updateCounter == updateMultiple)
            {
                Serial.print(F("\n\nTEST "));
                Serial.println(String(i));
                
                Serial.print(F("Delta: "));
                Serial.println(String(delta));
                
                Serial.print(F("Absolute distance: "));
                Serial.println(String(dist));
                
                Serial.print(F("Distance by getDistRemaining(): "));
                Serial.println(String(totalDistA));
                
                Serial.print(F("Distance by sum: "));
                Serial.println(String(totalDistB));
                
                Serial.print(F("Max Speed: "));
                Serial.print(String(speed));
                Serial.println(F(" Hz"));
                
                Serial.print(F("Top Speed (without correction): "));
                Serial.print(String(topSpeed));
                Serial.println(F(" Hz"));
                
                Serial.print(F("Accel/decel: "));
                Serial.print(String(accel));
                Serial.println(F(" Hz/s"));
                
                Serial.print(F("calcMaxAccelDist(): "));
                Serial.println(String(maxAccelDist));
                
                Serial.print(F("Accel distance: "));
                Serial.println(String(accelDist));
                
                Serial.print(F("Run distance: "));
                Serial.println(String(runDist));
                
                Serial.print(F("Decel distance: "));
                Serial.println(String(decelDist));
                
                Serial.print(F("Accel time (s): "));
                Serial.println(accelTime,10);
                
                Serial.print(F("Run time (s): "));
                Serial.println(runTime,10);
                
                Serial.print(F("Decel time (s): "));
                Serial.println(decelTime,10);
                
                Serial.print(F("Move time (s): "));
                Serial.println((accelTime + runTime + decelTime),10);
                
                updateCounter = 0;
                
                Serial.print(F("\n"));
                
            }
            
            updateCounter++;
            lastTime += updateInterval;
        }
        if (failed) break;

    }

    if (failed)
        Serial.println(F("\n\nAN ERRROR OCCURRED IN THE ABOVE PROFILE"));

    mot.stop();
    mot.setPos(prevPos);

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
        mot.setAccel(0);

        mot.prepareMove(testDist);
        Serial.print(F("Testing "));
        Serial.print(String(testSpeed));
        Serial.print(F(" Hz ("));
        Serial.print(String(mot.getAccelDist()));
        Serial.print(F("/"));
        Serial.print(String(mot.getRunDist()));
        Serial.print(F("/"));
        Serial.print(String(mot.getDecelDist()));
        Serial.print(F("): "));

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

void accelTest(void)
{
    // basic acceleration tests
    resetMotor();
    uint16_t peakSpeed;
    int32_t posB, posC;
    uint32_t accelCount, constCount, decelCount;
    uint32_t accelTime, constTime, decelTime;
    uint32_t startTime;

    Serial.println(F("\nBasic acceleration test: triangular profile"));

    const uint8_t TESTACCELCOUNT = 8;
    const uint16_t testAccels[TESTACCELCOUNT] = {400,800,1600,3200,6400,12800,25600,65535};

    for (uint8_t i = 0; i < TESTACCELCOUNT; i++)
    {
        uint16_t testAccel = testAccels[i];
        mot.setAccel(testAccel);
        mot.setMaxSpeed(testAccel); // one second to max speed

        // move to 0
        mot.prepareMove(0);
        while (mot.move() != STATE_STOPPED);

        // move twice the acceleration distance
        mot.prepareMove(mot.calcMaxAccelDist()*2);

        Serial.print(F("\nTesting "));
        Serial.print(String(testAccel));
        Serial.print(F(" Hz/s ("));
        Serial.print(String(mot.getAccelDist()));
        Serial.print(F("/"));
        Serial.print(String(mot.getRunDist()));
        Serial.print(F("/"));
        Serial.print(String(mot.getDecelDist()));
        Serial.println(F(")"));

        bool constSpeed = (mot.getRunDist() > 0);

        // accelerate
        // state will switch to STATE_RUN or STATE_DECEL on the last step of STATE_ACCEL
        mot.move(); // first call of move() initializes timings
        accelCount = 1;
        startTime = micros();
        while (mot.move() == STATE_ACCEL)
            accelCount++;
        accelTime = micros() - startTime; // us
        peakSpeed = mot.getCurSpeed();
        posB = mot.getPos();

        // constant speed
        // state will switch to STATE_STOPPED or STATE_DECEL on the last step of STATE_RUN
        constCount = 0;
        startTime = micros();
        if (mot.getState() == STATE_RUN)
        {
            while (mot.move() == STATE_RUN)
                constCount++;
            constCount++;
        }
        constTime = micros() - startTime; // us
        posC = mot.getPos();

        // decelerate
        // state will switch to STATE_STOPPED on the last step of STATE_DECEL
        decelCount = 1;
        startTime = micros();
        while (mot.move() == STATE_DECEL)
            decelCount++;
        decelTime = micros() - startTime; // us

        // report results
        Serial.print(F("Acc: "));
        float measuredAccel = (peakSpeed * 1000000.0) / accelTime;
        Serial.print(F("0 Hz - "));
        Serial.print(String(peakSpeed));
        Serial.print(F(" Hz @ "));
        Serial.print(String(measuredAccel));
        Serial.print(F(" Hz/s, "));
        Serial.print(String((float)(accelTime) / accelCount));
        Serial.println(" us per iteration.");

        if (constSpeed)
        {
            Serial.print(F("Con: "));
            float measuredSpeed = ((posC - posB) * 1000000.0) / constTime;
            Serial.print(String(measuredSpeed));
            Serial.print(F(" Hz, "));
            Serial.print(String((float)(constTime) / constCount));
            Serial.println(" us per iteration.");
        }

        Serial.print(F("Dec: "));
        float measuredDecel = (peakSpeed * 1000000.0) / decelTime;
        Serial.print(String(peakSpeed));
        Serial.print(F(" Hz - 0 Hz @ "));
        Serial.print(String(measuredDecel));
        Serial.print(F(" Hz/s, "));
        Serial.print(String((float)(decelTime) / decelCount));
        Serial.println(" us per iteration.");

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

    // conduct some tests during and after accelerating the motor forwards through one revolution

    Serial.println(F("\nMotor in motion with acceleration:\n----------------------------------"));
    Serial.println(F("\nDuring movement:"));

    mot.setAccel(REVOLUTION_PULSES);
    mot.prepareMove(REVOLUTION_PULSES);
    mot.move();
    int32_t targetDuring = mot.getTarget();
    bool forwardsDuring = mot.isMovingForwards();
    bool enabledDuring = mot.isEnabled();
    kissState_t stateDuring = mot.getState();
    while (mot.move() != STATE_STOPPED); // let motor complete motion

    // target should == REVOLUTION_PULSES
    if (targetDuring == REVOLUTION_PULSES)
    {
        Serial.print(F("getTarget() == "));
        Serial.print(String(REVOLUTION_PULSES));
        if (targetDuring == REVOLUTION_PULSES) Serial.println(F(" (pass)"));
    }
    else
    {
        Serial.print(F("getTarget() != "));
        Serial.print(String(REVOLUTION_PULSES));
        if (targetDuring == REVOLUTION_PULSES) Serial.println(F(" (pass)"));
    }

    delay(100);

    // forwards should be true
    if (forwardsDuring) Serial.println(F("isMovingForwards() == TRUE (pass)"));
    else Serial.println(F("isMovingForwards() != TRUE (fail)"));

    delay(100);

    // should be enabled
    if (enabledDuring) Serial.println(F("isEnabled() == TRUE (pass)"));
    else Serial.println(F("isEnabled() != TRUE (fail)"));

    delay(100);

    // state should == STATE_ACCEL
    if (stateDuring != STATE_STOPPED) Serial.println(F("getState() == STATE_ACCEL (pass)"));
    else Serial.println(F("getState() != STATE_ACCEL (fail)"));

    Serial.println(F("\nAfter movement:"));

    if (mot.getPos() == REVOLUTION_PULSES)
    {
        Serial.print(F("getPos() == "));
        Serial.print(String(REVOLUTION_PULSES));
        Serial.println(F(" (pass)"));
    }
    else
    {
        Serial.print(F("getPos() != "));
        Serial.print(String(REVOLUTION_PULSES));
        Serial.println(F(" (fail)"));
    }

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

    delay(100);

    // conduct some tests during and after moving the motor backwards through one revolution, without acceleration

    Serial.println(F("\nMotor in motion without acceleration:\n-------------------------------------"));
    Serial.println(F("\nDuring movement:"));

    mot.setAccel(0);
    mot.prepareMove(0);
    mot.move();
    targetDuring = mot.getTarget();
    forwardsDuring = mot.isMovingForwards();
    enabledDuring = mot.isEnabled();
    stateDuring = mot.getState();
    while (mot.move() != STATE_STOPPED); // let motor complete motion

    // target should == 0
    if (targetDuring == 0) Serial.println(F("getTarget() == 0 (pass)"));
    else Serial.println(F("getTarget() != 0 (fail)"));

    delay(100);

    // forwards should == FALSE
    if (!forwardsDuring) Serial.println(F("isMovingForwards() == FALSE (pass)"));
    else Serial.println(F("isMovingForwards() == TRUE (fail)"));

    delay(100);

    // should be enabled
    if (enabledDuring) Serial.println(F("isEnabled() == TRUE (pass)"));
    else Serial.println(F("isEnabled() == FALSE (fail)"));

    delay(100);

    // state should == STATE_RUN
    if (stateDuring == STATE_RUN) Serial.println(F("getState() == STATE_RUN (pass)"));
    else Serial.println(F("getState() != STATE_RUN (fail)"));

    Serial.println(F("\nAfter movement:"));

    if (mot.getPos() == 0) Serial.println(F("getPos() == 0 (pass)"));
    else Serial.println(F("getPos() != 0 (fail)"));

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
    mot.setAccel(0);
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
    Serial.println(F("<profiles> test speed profiles against some simple rules"));
    Serial.println(F("<speed> tests speed accuracy and timing"));
    Serial.println(F("<accel> tests accleration accuracy and timing"));
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
            if (command == "profiles") speedProfiles();
            else if (command == "speed") speedTest();
            else if (command == "accel") accelTest();
            else if (command == "states") states();
            else if (command == "limits") limits();
            else if (command == "menu") menuPrinted = false;
            command = "";
        }
        else if (commandIn) command += c;
    }
}

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

void setup(void)
{

    // initialize the kissStepper
    mot.begin();
    
    // set drive mode pins
    // the kissStepper library does not do this for you!
    pinMode(PIN_MS1, OUTPUT);
    pinMode(PIN_MS2, OUTPUT);
    digitalWrite(PIN_MS1, LOW);
    digitalWrite(PIN_MS2, LOW);
    switch (DRIVE_MODE)
    {
    case 2: // half-step
        digitalWrite(PIN_MS1, HIGH);
        break;
    case 4: // quarter-step
        digitalWrite(PIN_MS2, HIGH);
        break;
    case 8: // eighth-step
        digitalWrite(PIN_MS1, HIGH);
        digitalWrite(PIN_MS2, HIGH);
        break;
    }
    
    Serial.begin(9600);
}
