/*

This software is intended for testing the kissStepper library
It measures timing and attempts to cause intentional malfunctions
Although it will not test all possible things, it is a good start
Always run this test after making changes to the library

Written by Rylee Isitt
September 19, 2015

This software is licensed under the GPL v3

*/
#include <kissStepper.h>

// instantiate an instance of the kissStepper class for an Easy Driver
kissStepper mot(7, 3, 4, 5, 6);

const uint16_t motorFullStPerRev = 200; // number of full steps in one revolution of the test motor
const uint16_t motorOneRev = motorFullStPerRev * kissStepper::FULL; // number of microsteps in one revolution of the test motor

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

void resetMotor(void)
{
    delay(100);
    mot.hardStop();
    mot.setAccel(0);
    mot.setMaxRPM(60);
    mot.setDriveMode(kissStepper::MICRO8);
    mot.moveTo(0);
    while (mot.work());
    delay(100);
}

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

void setMotorDriveMode(uint8_t mode)
{
    switch(mode)
    {
    case 1:
        mot.setDriveMode(kissStepper::FULL);
        break;
    case 2:
        mot.setDriveMode(kissStepper::HALF);
        break;
    case 4:
        mot.setDriveMode(kissStepper::MICRO4);
        break;
    case 8:
        mot.setDriveMode(kissStepper::MICRO8);
        break;
    }
}

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

void basic(void)
{
    // basic speed and drive mode tests
    resetMotor();
    Serial.println(F("\nBasic speed & mode test"));
    for (byte RPM = 15; RPM <= 60; RPM*=2)
    {
        for (byte mode = 1; mode <= 8; mode*=2)
        {
            Serial.print(F("\nSet RPM: "));
            Serial.print(String(RPM));
            Serial.print(F(" ("));
            Serial.print(F("1/"));
            Serial.print(String(mode));
            Serial.println(F(" step)"));
            mot.setMaxRPM(RPM);
            setMotorDriveMode(mode);
            if (mot.getPos() == 0) mot.moveTo(motorOneRev);
            else mot.moveTo(0);
            uint32_t startTime = micros();
            while (mot.work());
            uint32_t endTime = micros();
            float measuredRPM = 60000000.0 / (float)(endTime - startTime);
            Serial.print(F("Measured RPM: "));
            Serial.println(String(measuredRPM));
            delay(100);
        }
    }
}

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

void benchmark(void)
{
    uint32_t startTime = micros();
    uint32_t endTime = micros();
    Serial.println(F("\nwork() benchmark without acceleration"));
    for (byte RPM = 15; RPM <= 60; RPM*=2)
    {
        resetMotor();
        mot.setMaxRPM(RPM);
        Serial.print(F("Testing "));
        Serial.print(String(RPM));
        Serial.print(F("RPM: "));
        uint32_t runCount = 0;
        mot.moveTo(motorOneRev);
        startTime = micros();
        while (mot.work()) runCount++;
        endTime = micros();
        float runTime = (float)(endTime - startTime) / runCount;
        Serial.print(String(runTime));
        Serial.print(" us avg over ");
        Serial.print(String(runCount));
        Serial.println(" calls");
    }
    Serial.println(F("\nwork() benchmark with 60RPM/s acceleration"));
    for (byte RPM = 15; RPM <= 60; RPM*=2)
    {
        resetMotor();
        mot.setAccel(60);
        mot.setMaxRPM(RPM);
        Serial.print(F("Testing "));
        Serial.print(String(RPM));
        Serial.print(F("RPM: "));
        uint32_t runCount = 0;
        mot.moveTo(motorOneRev);
        startTime = micros();
        while (mot.work()) runCount++;
        endTime = micros();
        float runTime = (float)(endTime - startTime) / runCount;
        Serial.print(String(runTime));
        Serial.print(" us avg over ");
        Serial.print(String(runCount));
        Serial.println(" calls");
    }
}

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

void fracRPM(void)
{
    resetMotor();
    Serial.println(F("\nFractional RPM test"));
    for (uint16_t RP10M = 600; RP10M <= 609; RP10M += 1)
    {
        Serial.print(F("\nSet RPM: "));
        Serial.println(String((float)(RP10M/10.0)));
        mot.setMaxRP10M(RP10M);
        if (mot.getPos() == 0) mot.moveTo(motorOneRev);
        else mot.moveTo(0);
        uint32_t startTime = micros();
        while (mot.work());
        uint32_t endTime = micros();
        float measuredRPM = 60000000.0 / (float)(endTime - startTime);
        Serial.print(F("Measured RPM: "));
        Serial.println(String(measuredRPM));
        delay(100);
    }
}

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

void rpmchange(void)
{
    // test changing of speed while moving (no acceleration)
    resetMotor();
    Serial.println(F("\nSpeed change test"));
    mot.moveTo(mot.forwardLimit);
    uint32_t testPos = 0;
    for (byte RPM = 15; RPM <= 60; RPM*=2)
    {
        testPos += motorOneRev;
        Serial.print("\nSet RPM: ");
        Serial.println(String(RPM));
        mot.setMaxRPM(RPM);
        uint32_t startTime = micros();
        while (mot.getPos() != testPos) mot.work();
        uint32_t endTime = micros();
        float measuredRPM = 60000000.0 / (float)(endTime - startTime);
        Serial.print("Measured RPM: ");
        Serial.println(String(measuredRPM));
    }
    mot.stop();
}

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

void modechange(void)
{
    // test changing of drive mode while moving (no acceleration)
    resetMotor();
    Serial.println(F("\nMode change test (should have constant RPM)"));
    mot.setMaxRPM(60);
    mot.moveTo(mot.forwardLimit);
    uint32_t testPos = 0;
    for (byte mode = 1; mode <= 8; mode*=2)
    {
        testPos += motorOneRev;
        setMotorDriveMode(mode);
        Serial.print("1/");
        Serial.print(String(mode));
        Serial.print(" step:");
        uint32_t startTime = micros();
        while (mot.getPos() != testPos) mot.work();
        uint32_t endTime = micros();
        float measuredRPM = 60000000.0 / (float)(endTime - startTime);
        Serial.print(String(measuredRPM));
        Serial.println("RPM");
    }
    mot.stop();
}

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

void accel(void)
{
    // test changing of speed & accel while moving
    uint32_t startTime;
    uint32_t endTime;
    uint32_t startPos;
    uint32_t endPos;
    resetMotor();
    mot.setAccel(60);
    mot.moveTo(mot.forwardLimit);
    Serial.println(F("\nSpeed & accel test"));
    float startRPM = 0.0;
    float endRPM = 0.0;

    byte RPMS;
    for (RPMS = 60; RPMS >= 15; RPMS = RPMS >> 1)
    {
        for (byte RPM = 15; RPM <= 60; RPM = RPM << 1)
        {
            startRPM = endRPM;
            mot.setAccel(RPMS);
            mot.setMaxRPM(RPM);

            Serial.print("\nGoto ");
            Serial.print(String(RPM));
            Serial.print(" RPM @ ");
            Serial.print(String(RPMS));
            Serial.println(" RPM/s");

            // acceleration phase
            startTime = micros();
            do
            {
                mot.work();
            }
            while (mot.getAccelState() != 0);
            endTime = micros();
            uint32_t accelTime = endTime - startTime;

            // constant speed phase
            // move foward one revolution
            startPos = mot.getPos();
            startTime = micros();
            while (mot.getPos() != (startPos+motorOneRev)) mot.work();
            endTime = micros();
            endPos = mot.getPos();
            endRPM = 60000000.0 / (float)(endTime - startTime);
            float measuredRPMS = ((endRPM - startRPM) * 1000000.0) / accelTime;

            Serial.print("From ");
            Serial.print(String(startRPM));
            Serial.print(" RPM to ");
            Serial.print(String(endRPM));
            Serial.print(" RPM @ ");
            Serial.print(String(measuredRPMS));
            Serial.println(" RPM/s");
        }
    }
    Serial.println("\nStop");
    mot.stop();
    startRPM = endRPM;
    startTime = micros();
    while (mot.work());
    endTime = micros();
    endRPM = 0.0;
    float measuredRPMS = ((endRPM - startRPM) * 1000000.0) / (float)(endTime - startTime);
    Serial.print("From ");
    Serial.print(String(startRPM));
    Serial.print(" RPM to ");
    Serial.print(String(endRPM));
    Serial.print(" RPM @ ");
    Serial.print(String(measuredRPMS));
    Serial.println(" RPM/s");
}

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

void misc(void)
{
    resetMotor();
    mot.disable();
    Serial.println(F("\nMotor stationary and disabled:"));

    if (mot.getTarget() == mot.getPos()) Serial.println(F("getTarget() == getPos() (pass)"));
    else Serial.println(F("getTarget() != getPos() (fail)"));

    delay(100);

    if (mot.getCurRP10M() == 0) Serial.println(F("getCurRP10M() == 0 (pass)"));
    else Serial.println(F("getCurRP10M() != 0 (fail)"));

    delay(100);

    if (!mot.isMoving()) Serial.println(F("isMoving() == FALSE (pass)"));
    else Serial.println(F("isMoving() == TRUE (fail)"));

    delay(100);

    if (!mot.isEnabled()) Serial.println(F("isEnabled() == FALSE (pass)"));
    else Serial.println(F("isEnabled() == TRUE (fail)"));

    delay(100);

    if (mot.getAccelState() == 0) Serial.println(F("getAccelState() == 0 (pass)"));
    else Serial.println(F("getAccelState() != 0 (fail)"));

    delay(100);

    mot.setAccel(60);
    mot.moveTo(motorOneRev);
    mot.work();

    Serial.println(F("\nMotor in motion with acceleration:"));

    if (mot.getTarget() != mot.getPos()) Serial.println(F("getTarget() != getPos() (pass)"));
    else Serial.println(F("getTarget() == getPos() (fail)"));

    delay(100);

    if (mot.isMoving()) Serial.println(F("isMoving() == TRUE (pass)"));
    else Serial.println(F("isMoving() == FALSE (fail)"));

    delay(100);

    if (mot.isEnabled()) Serial.println(F("isEnabled() == TRUE (pass)"));
    else Serial.println(F("isEnabled() == FALSE (fail)"));

    delay(100);

    if (mot.getAccelState() != 0) Serial.println(F("getAccelState() != 0 (pass)"));
    else Serial.println(F("getAccelState() == 0 (fail)"));

    // let motor move
    while (mot.work());

    Serial.println(F("\nAfter movement:"));

    if (mot.getPos() == motorOneRev)
    {
        Serial.print(F("getPos() == "));
        Serial.print(String(motorOneRev));
        Serial.println(F(" (pass)"));
    }
    else
    {
        Serial.print(F("getPos() != "));
        Serial.print(String(motorOneRev));
        Serial.println(F(" (fail)"));
    }

    delay(100);

    if (mot.getTarget() == mot.getPos()) Serial.println(F("getTarget() == getPos() (pass)"));
    else Serial.println(F("getTarget() != getPos() (fail)"));

    delay(100);

    if (mot.getCurRP10M() == 0) Serial.println(F("getCurRP10M() == 0 (pass)"));
    else Serial.println(F("getCurRP10M() != 0 (fail)"));

    delay(100);

    if (!mot.isMoving()) Serial.println(F("isMoving() == FALSE (pass)"));
    else Serial.println(F("isMoving() == TRUE (fail)"));

    delay(100);

    if (mot.isEnabled()) Serial.println(F("isEnabled() == TRUE (pass)"));
    else Serial.println(F("isEnabled() == FALSE (fail)"));

    delay(100);

    if (mot.getAccelState() == 0) Serial.println(F("getAccelState() == 0 (pass)"));
    else Serial.println(F("getAccelState() != 0 (fail)"));

    delay(100);

    mot.setAccel(0);
    mot.moveTo(0);
    mot.work();

    Serial.println(F("\nMotor in motion without acceleration:"));

    if (mot.getTarget() != mot.getPos()) Serial.println(F("getTarget() != getPos() (pass)"));
    else Serial.println(F("getTarget() == getPos() (fail)"));

    delay(100);

    if (mot.isMoving()) Serial.println(F("isMoving() == TRUE (pass)"));
    else Serial.println(F("isMoving() == FALSE (fail)"));

    delay(100);

    if (mot.isEnabled()) Serial.println(F("isEnabled() == TRUE (pass)"));
    else Serial.println(F("isEnabled() == FALSE (fail)"));

    delay(100);

    if (mot.getAccelState() == 0) Serial.println(F("getAccelState() == 0 (pass)"));
    else Serial.println(F("getAccelState() != 0 (fail)"));

    // let motor move
    mot.stop();
    mot.moveTo(0);
    while (mot.work());

    Serial.println(F("\nAfter returning to 0:"));

    if (mot.getPos() == 0) Serial.println(F("getPos() == 0 (pass)"));
    else Serial.println(F("getPos() != 0 (fail)"));

    delay(100);

    if (mot.getTarget() == mot.getPos()) Serial.println(F("getTarget() == getPos() (pass)"));
    else Serial.println(F("getTarget() != getPos() (fail)"));

    delay(100);

    if (mot.getCurRP10M() == 0) Serial.println(F("getCurRP10M() == 0 (pass)"));
    else Serial.println(F("getCurRP10M() != 0 (fail)"));

    delay(100);

    if (!mot.isMoving()) Serial.println(F("isMoving() == FALSE (pass)"));
    else Serial.println(F("isMoving() == TRUE (fail)"));

    delay(100);

    if (mot.isEnabled()) Serial.println(F("isEnabled() == TRUE (pass)"));
    else Serial.println(F("isEnabled() == FALSE (fail)"));

    delay(100);

    if (mot.getAccelState() == 0) Serial.println(F("getAccelState() == 0 (pass)"));
    else Serial.println(F("getAccelState() != 0 (fail)"));

}

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

void limits(void)
{
    resetMotor();
    Serial.println(F("\nSetting limits -1000/+1000:"));
    mot.forwardLimit = 1000;
    mot.reverseLimit = -1000;

    mot.moveTo(10000);
    if (mot.getTarget() == mot.forwardLimit) Serial.println(F("moveTo() forward limit PASS"));
    else Serial.println(F("moveTo() forward limit FAIL"));

    delay(100);

    mot.moveTo(-10000);
    if (mot.getTarget() == mot.reverseLimit) Serial.println(F("moveTo() reverse limit PASS"));
    else Serial.println(F("moveTo() reverse limit FAIL"));

    delay(100);

    mot.setPos(10000);
    if (mot.getPos() == mot.forwardLimit) Serial.println(F("setPos() forward limit PASS"));
    else Serial.println(F("setPos() forward limit FAIL"));

    delay(100);

    mot.setPos(-10000);
    if (mot.getPos() == mot.reverseLimit) Serial.println(F("setPos() reverse limit PASS"));
    else Serial.println(F("setPos() reverse limit FAIL"));

    mot.forwardLimit = 2147483647L;
    mot.reverseLimit = -2147483648L;
    mot.setPos(0);

}

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

void loop(void)
{

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
            if (command == "basic") basic();
            else if (command == "benchmark") benchmark();
            else if (command == "fracrpm") fracRPM();
            else if (command == "rpmchange") rpmchange();
            else if (command == "modechange") modechange();
            else if (command == "accel") accel();
            else if (command == "misc") misc();
            else if (command == "limits") limits();
            else if (command == "all")
            {
                basic();
                benchmark();
                fracRPM();
                rpmchange();
                modechange();
                accel();
                misc();
                limits();
            }
            command = "";
        }
        else if (commandIn) command += c;
    }
}

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

void setup(void)
{

    // test stop()
    // test hardStop()
    // test that extents work
    // curRPM should always be 0 if motor is not moving, even after calling setMaxRPM with acceleration on or off
    // check that getMaxRPM() returns same number provided to setMaxRPM()
    // do the above with changes to drive mode, setMaxRP10M()

    Serial.begin(9600);

    // initialize the kissStepper
    // the motor has 200 steps, use 1/8th drive mode, and set the maximum speed to 60 RPM
    mot.begin(motorFullStPerRev, kissStepper::MICRO8, 60);


    Serial.println(F("Usage:"));
    Serial.println(F("<basic> to run a basic test using different speeds and drive modes"));
    Serial.println(F("<benchmark> to test the execution time of the work() method"));
    Serial.println(F("<fracrpm> to test the the accuracy of fractional RPM speeds"));
    Serial.println(F("<rpmchange> to test changing of speed while moving"));
    Serial.println(F("<modechange> to test changing of drive mode while moving"));
    Serial.println(F("<accel> to test acceleration"));
    Serial.println(F("<misc> to test getters/setters and status variables"));
    Serial.println(F("<limits> to test the forward and reverse limits"));
    Serial.println(F("<all> runs all of the above tests"));

}
