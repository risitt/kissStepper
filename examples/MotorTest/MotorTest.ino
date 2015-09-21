/*

This software is intended for testing the kissStepper library
It measures timing and attempts to cause intentional malfunctions
Although it will not test all possible things, it is a good start
Always run this test after making changes to the library

Written by Rylee Isitt
September 19, 2015

This software is licensed under the GPL

*/
#include <kissStepper.h>

// instantiate an instance of the kissStepper class for an Easy Driver
kissStepper mot(7, 3, 4, 5, 6);

const uint16_t motorFullStPerRev = 200; // number of full steps in one revolution of the test motor
const uint16_t motorSixteenthStPerRev = motorFullStPerRev * 16; // number of 1/16th steps in one revolution of the test motor

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

void resetMotor(void)
{
	delay(100);
	mot.hardStop();
	mot.setAccel(0);
	mot.setMaxRPM(60);
	mot.setDriveMode(kissStepper::EIGHTH);
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
		mot.setDriveMode(kissStepper::QUARTER);
		break;
	case 8:
		mot.setDriveMode(kissStepper::EIGHTH);
		break;
	}
}

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

void motorTest(void)
{

	uint32_t startTime = micros();
	uint32_t endTime = micros();
	uint32_t startPos = 0;
	uint32_t endPos = 0;
	uint32_t testPos = 0;

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
			if (mot.getPos() == 0) mot.moveTo(motorSixteenthStPerRev);
			else mot.moveTo(0);
            startTime = micros();
            while (mot.work());
            endTime = micros();
            float measuredRPM = 60000000.0 / (float)(endTime - startTime);
            Serial.print(F("Measured RPM: "));
            Serial.println(String(measuredRPM));
			delay(100);
        }
    }
	
	// fractional RPM tests
	resetMotor();
    Serial.println(F("\nFractional RPM test"));
    for (uint16_t RP10M = 600; RP10M <= 609; RP10M += 1)
    {
		Serial.print(F("\nSet RPM: "));
		Serial.println(String((float)(RP10M/10.0)));
		mot.setMaxRP10M(RP10M);
		if (mot.getPos() == 0) mot.moveTo(motorSixteenthStPerRev);
		else mot.moveTo(0);
		startTime = micros();
		while (mot.work());
		endTime = micros();
		float measuredRPM = 60000000.0 / (float)(endTime - startTime);
		Serial.print(F("Measured RPM: "));
		Serial.println(String(measuredRPM));
		delay(100);
	}
	
	// test changing of speed while moving (no acceleration)
	resetMotor();
	Serial.println(F("\nSpeed change test"));
	mot.moveTo(mot.forwardLimit);
	testPos = 0;
	for (byte RPM = 15; RPM <= 60; RPM*=2)
	{
		testPos += 3200;
		Serial.print("\nSet RPM: ");
		Serial.println(String(RPM));
		mot.setMaxRPM(RPM);
		startTime = micros();
		while (mot.getPos() != testPos) mot.work();
		endTime = micros();
		float measuredRPM = 60000000.0 / (float)(endTime - startTime);
		Serial.print("Measured RPM: ");
		Serial.println(String(measuredRPM));
	}
	mot.stop();
	
	// test changing of drive mode while moving (no acceleration)
	resetMotor();
	Serial.println(F("\nMode change test (should have constant RPM)"));
	mot.setMaxRPM(60);
	mot.moveTo(mot.forwardLimit);
	testPos = 0;
	for (byte mode = 1; mode <= 8; mode*=2)
	{
		testPos += 3200;
		setMotorDriveMode(mode);
		Serial.print("1/");
		Serial.print(String(mode));
		Serial.print(" step:");
		startTime = micros();
		while (mot.getPos() != testPos) mot.work();
		endTime = micros();
		float measuredRPM = 60000000.0 / (float)(endTime - startTime);
		Serial.print(String(measuredRPM));
		Serial.println("RPM");
	}
	mot.stop();
	
	// test changing of speed & accel while moving
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
			} while (mot.isAccelerating());
			endTime = micros();
			uint32_t accelTime = endTime - startTime;

			// constant speed phase
			// move foward one revolution
			startPos = mot.getPos();
			startTime = micros();
			while (mot.getPos() != (startPos+motorSixteenthStPerRev)) mot.work();
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
	
	
	// test stop()
	// test hardStop()
	// test that extents work
	// test that enable() is automatically called
	// curRPM should always be 0 if motor is not moving, even after calling setMaxRPM with acceleration on or off
	// check that getMaxRPM() returns same number provided to setMaxRPM()
	// do the above with changes to drive mode, setMaxRP10M()
	
	
	// work() benchmark
	Serial.println(F("\nwork() benchmark without acceleration"));
    for (byte RPM = 15; RPM <= 60; RPM*=2)
    {
		resetMotor();
		mot.setMaxRPM(RPM);
		Serial.print(F("Testing "));
		Serial.print(String(RPM));
		Serial.print(F("RPM: "));
		uint32_t runCount = 0;
		mot.moveTo(3200);
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
		mot.moveTo(3200);
		startTime = micros();
		while (mot.work()) runCount++;
		endTime = micros();
		float runTime = (float)(endTime - startTime) / runCount;
		Serial.print(String(runTime));
		Serial.print(" us avg over ");
		Serial.print(String(runCount));
		Serial.println(" calls");
	}
	
	// return motor to start
	resetMotor();
	
	Serial.println("\nTests complete. Is the motor at 0?");

}

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

void loop(void)
{

	// wait for the start command
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
            if (command == "start") motorTest();
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
    // the motor has 200 steps, use 1/8th drive mode, and set the maximum speed to 60 RPM
    mot.begin(motorFullStPerRev, kissStepper::EIGHTH, 60);

    Serial.begin(9600);
    Serial.println(F("To start, type <start>"));
    Serial.println(F("After all tests are complete, check that the motor has returned to the initial position."));

}
