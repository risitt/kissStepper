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

// instantiate the kissStepper class for an Easy Driver
kissStepper mot(
	kissPinAssignments(3, 4, 7, 5, 6),
	kissMicrostepConfig(MICROSTEP_8)
	);

const uint16_t motorFullStPerRev = 200; // number of full steps in one revolution of the test motor
const uint16_t motorOneRev = motorFullStPerRev * mot.fullStepVal; // number of microsteps in one revolution of the test motor
	
// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

String getSerialCommand(void)
{
    String command = "";
    bool commandIn = false;
    while (Serial.available() > 0)
    {
        char c = Serial.read();
        if (c == '<') commandIn = true;
        else if (c == '>')
        {
            command.trim();
            command.toLowerCase();
			return command;
        }
        else if (commandIn) command += c;
    }
}

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

void resetMotor(void)
{
    delay(100);
    mot.stop();
    mot.setAccel(0);
    mot.setMaxSpeed(200);
    mot.setDriveMode(MICROSTEP_8);
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
        mot.setDriveMode(FULL_STEP);
        break;
    case 2:
        mot.setDriveMode(HALF_STEP);
        break;
    case 4:
        mot.setDriveMode(MICROSTEP_4);
        break;
    case 8:
        mot.setDriveMode(MICROSTEP_8);
        break;
    }
}

// ----------------------------------------------------------------------------------------------------
// Measures motor acceleration
// ----------------------------------------------------------------------------------------------------

void measureAccel(void)
{
	uint32_t startTime, accelTime;
	
	// acceleration phase
	uint16_t startSpeed = mot.getCurSpeed();
	if (mot.getAccel() != 0)
	{
		startTime = micros();
		do
		{
			mot.work();
		}
		while (mot.getAccelState() != 0);
		accelTime = micros() - startTime;
	}
	else
	{
		accelTime = 0;
	}
	uint16_t endSpeed = mot.getCurSpeed();
	
	// calculate acceleration
	float measuredAccel = (((int32_t)endSpeed - startSpeed) * 1000000.0) / accelTime;
	
	// send results
	Serial.print(String(startSpeed));
	Serial.print(F("-"));
	Serial.print(String(endSpeed));
	Serial.print(F(" st/s @ "));
	Serial.print(String(measuredAccel));
	Serial.print(F(" st/s^2 ("));
	Serial.print(String(mot.getAccel()));
	Serial.println(F(" st/s^2)"));
	
}

// ----------------------------------------------------------------------------------------------------
// Measures motor speed over a certain distance
// ----------------------------------------------------------------------------------------------------

void measureSpeed(uint32_t overDist)
{
	int32_t startPos = mot.getPos();
	int32_t targetPos = mot.getTarget();
	int32_t testEndPos = (targetPos > startPos) ? (startPos + overDist) : (startPos - overDist);
	uint32_t startTime, endTime;
	
	uint16_t startSpeed = mot.getCurSpeed();
	startTime = micros();
	while (mot.work())
	{
		if ((targetPos > startPos) && (mot.getPos() >= testEndPos)) break;
		else if ((targetPos < startPos) && (mot.getPos() <= testEndPos)) break;
	}
	endTime = micros();
	uint16_t endSpeed = mot.getCurSpeed();
	int32_t endPos = mot.getPos();
	
	// calculate speed
	int32_t dist = endPos - startPos; // microsteps
	uint32_t time = endTime - startTime; // us
	float measuredSpeed = (1000000.0 * dist) / (time * mot.fullStepVal);
	
	Serial.print(String(measuredSpeed));
	Serial.print(F(" st/s avg ("));
	if (startSpeed == endSpeed)
		Serial.print(String(startSpeed));
	else
	{
		Serial.print(String(startSpeed));
		Serial.print(F(" - "));
		Serial.print(String(endSpeed));
	}
	Serial.println(F(" st/s)"));

}

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

void basic(void)
{
    // basic speed and drive mode tests
    resetMotor();
	mot.moveTo(mot.forwardLimit);
    Serial.println(F("\nBasic speed & mode test"));
	for (uint8_t mode = 1; mode <= 8; mode = mode << 1)
	{
		Serial.print(F("\n1/"));
		Serial.print(String(mode));
		Serial.println(F(" step:"));
		setMotorDriveMode(mode);
		for (uint16_t stPerSec = 50; stPerSec <= 200; stPerSec = stPerSec << 1)
		{
            mot.setMaxSpeed(stPerSec);
			measureSpeed(motorOneRev);
        }
    }
	mot.stop();
}

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

void benchmark(void)
{
    uint32_t startTime = micros();
    uint32_t endTime = micros();
    Serial.println(F("\nwork() benchmark without acceleration"));
    for (uint16_t stPerSec = 50; stPerSec <= 200; stPerSec*=2)
    {
        resetMotor();
        mot.setMaxSpeed(stPerSec);
        Serial.print(F("Testing "));
        Serial.print(String(stPerSec));
        Serial.print(F(" st/s: "));
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
    Serial.println(F("\nwork() benchmark with 200 st/s^2 acceleration"));
    for (uint16_t stPerSec = 50; stPerSec <= 200; stPerSec*=2)
    {
        resetMotor();
        mot.setAccel(200);
        mot.setMaxSpeed(stPerSec);
        Serial.print(F("Testing "));
        Serial.print(String(stPerSec));
        Serial.print(F(" st/s: "));
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

void speedTest(void)
{
    resetMotor();
    Serial.println(F("\nSpeed Ramp Test From 50 to 400 st/s"));
	Serial.println(F("\nSend <s> to stop"));
	
    for (uint16_t stPerSec = 50; stPerSec <= 400; stPerSec++)
    {
		uint32_t testDist = mot.fullStepVal * stPerSec;
		mot.setMaxSpeed(stPerSec);
		mot.moveTo(mot.getPos() + testDist);
		uint32_t startTime = micros();
		while (mot.work()) {}
		uint32_t endTime = micros();
		float measuredSpeed = (1000000.0 * testDist) / ((endTime - startTime) * mot.fullStepVal);
		Serial.print(measuredSpeed,4);
		Serial.print(F(" st/s avg ("));
		Serial.print(String(stPerSec));
		Serial.println(F(" st/s)"));
		if (getSerialCommand() == "s") break;
    }
	mot.stop();
}

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

void speedChange(void)
{
    // test changing of speed while moving (no acceleration)
    resetMotor();
	mot.moveTo(mot.forwardLimit);
	
    Serial.println(F("\nSpeed change test"));
    for (uint16_t stPerSec = 50; stPerSec <= 200; stPerSec*=2)
    {
        mot.setMaxSpeed(stPerSec);
		measureSpeed(motorOneRev);
    }
    mot.stop();
}

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

void modechange(void)
{
    // test changing of drive mode while moving (no acceleration)
    resetMotor();
	mot.setMaxSpeed(200);
	mot.moveTo(mot.forwardLimit);
	
    Serial.println(F("\nMode change test"));
    for (byte mode = 1; mode <= 8; mode*=2)
    {
        setMotorDriveMode(mode);
        Serial.print("1/");
        Serial.print(String(mode));
        Serial.println(" step");
		measureSpeed(motorOneRev);
    }
    mot.stop();
}

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

void accel(void)
{
    // test changing of speed & accel while moving

    resetMotor();
    mot.setAccel(200);
    mot.moveTo(mot.forwardLimit);
    Serial.println(F("\nSpeed & accel test"));

    for (uint8_t accel = 200; accel >= 50; accel = accel >> 1)
    {
        for (uint16_t stPerSec = 50; stPerSec <= 200; stPerSec*=2)
        {
            mot.setAccel(accel);
            mot.setMaxSpeed(stPerSec);

            // measurements
            measureAccel();
			measureSpeed(motorOneRev);
			Serial.println();
			
        }
    }
	
    mot.decelerate();
	measureAccel();
	mot.stop();
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

    if (mot.getCurSpeed() == 0) Serial.println(F("getCurSpeed() == 0 (pass)"));
    else Serial.println(F("getCurSpeed() != 0 (fail)"));

    delay(100);

    if (mot.getMoveState() == 0) Serial.println(F("getMoveState() == 0 (pass)"));
    else Serial.println(F("getMoveState() != 0 (fail)"));

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

    if (mot.getMoveState() != 0) Serial.println(F("getMoveState() != 0 (pass)"));
    else Serial.println(F("getMoveState() == 0 (fail)"));

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

    if (mot.getCurSpeed() == 0) Serial.println(F("getCurSpeed() == 0 (pass)"));
    else Serial.println(F("getCurSpeed() != 0 (fail)"));

    delay(100);

    if (mot.getMoveState() == 0) Serial.println(F("getMoveState() == 0 (pass)"));
    else Serial.println(F("getMoveState() != 0 (fail)"));

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

    if (mot.getMoveState() != 0) Serial.println(F("getMoveState() != 0 (pass)"));
    else Serial.println(F("getMoveState() == 0 (fail)"));

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

    if (mot.getCurSpeed() == 0) Serial.println(F("getCurSpeed() == 0 (pass)"));
    else Serial.println(F("getCurSpeed() != 0 (fail)"));

    delay(100);

    if (mot.getMoveState() == 0) Serial.println(F("getMoveState() == 0 (pass)"));
    else Serial.println(F("getMoveState() != 0 (fail)"));

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
	mot.stop();

    delay(100);

    mot.moveTo(-10000);
    if (mot.getTarget() == mot.reverseLimit) Serial.println(F("moveTo() reverse limit PASS"));
    else Serial.println(F("moveTo() reverse limit FAIL"));
	mot.stop();

    delay(100);

    mot.setPos(10000);
    if (mot.getPos() == mot.forwardLimit) Serial.println(F("setPos() forward limit PASS"));
    else Serial.println(F("setPos() forward limit FAIL"));
	mot.stop();

    delay(100);

    mot.setPos(-10000);
    if (mot.getPos() == mot.reverseLimit) Serial.println(F("setPos() reverse limit PASS"));
    else Serial.println(F("setPos() reverse limit FAIL"));
	mot.stop();

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
            else if (command == "speedtest") speedTest();
            else if (command == "speedchange") speedChange();
            else if (command == "modechange") modechange();
            else if (command == "accel") accel();
            else if (command == "misc") misc();
            else if (command == "limits") limits();
            else if (command == "all")
            {
                basic();
                benchmark();
                speedChange();
                modechange();
                accel();
                misc();
                limits();
				speedTest();
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

    Serial.begin(9600);

    // initialize the kissStepper
    // use 1/8th drive mode, and set the maximum speed to 200 st/s
    mot.begin(MICROSTEP_8, 200);
    Serial.println(F("Usage:"));
    Serial.println(F("<basic> tests different speeds and modes"));
    Serial.println(F("<benchmark> bechmarks work()"));
    Serial.println(F("<speedtest> a speed ramp with 1 st/s increments"));
    Serial.println(F("<speedchange> tests speed changes while moving"));
    Serial.println(F("<modechange> test mode changes while moving"));
    Serial.println(F("<accel> acceleration test"));
    Serial.println(F("<misc> tests misc. operations"));
    Serial.println(F("<limits> test position limits"));
    Serial.println(F("<all> runs all of the above"));
	Serial.println(F("\nSet values (in brackets) follow measured values\n"));
	Serial.flush();

}
