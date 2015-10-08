/*

Control an Easy Driver through a serial connection

Written by Rylee Isitt
September 14, 2015

The code in this file is released into the public domain.
Libraries are licensed separately (see their licenses for details).

*/
#include <kissStepper.h>

const uint16_t motorFullStPerRev = 200; // number of full steps in one revolution of the motor

// instantiate the kissStepper
// This sets up an Easy Driver with software control over DIR, STEP, ENABLE, MS1, and MS2 pins
// the pin assignments are in the order of DIR, STEP, ENABLE, MS1, and MS2, respectively
// The Easy Driver only supports up to 1/8th microstepping, so we set MICROSTEP_8 as the maxMicrostepMode
kissStepper mot(
	kissPinAssignments(3, 4, 7, 5, 6),
	kissMicrostepConfig(MICROSTEP_8)
	);


void loop(void)
{

	// this line below actually makes the motor move
	// and needs to be run repeatedly and often for smooth motion
	// you can do things this way, or you can put work() in a for/while loop
	// or you can call work() from a timer interrupt
	bool moving = mot.work();
	
	
	// get commands from serial
	static String command = "";
	static bool commandIn = false;
	while (Serial.available() > 0) {
		char c = Serial.read();
		if (c == '<') commandIn = true;
		else if (c == '>')
		{
			commandIn = false;
			command.trim();
			command.toLowerCase();
			byte splitAt = command.indexOf(' ');
			String key = command.substring(0, splitAt);
			String value = command.substring(splitAt+1);
			command = "";

			if (key == F("posunit"))
			{
				Serial.print(F("The position is measured in 1/"));
				Serial.print(String(mot.fullStepVal));
				Serial.println(F(" steps"));
			}
			else if (key == F("getpos"))
			{
				Serial.print(F("Position: "));
				Serial.println(String(mot.getPos()));
			}
			else if (key == F("target"))
			{
				Serial.print(F("Target: "));
				Serial.println(String(mot.getTarget()));
			}
			else if (key == F("ismoving"))
			{
				if (moving) Serial.println(F("TRUE"));
				else Serial.println(F("FALSE"));
			}
			else if (key == F("goto"))
			{
				int32_t targetPos = value.toInt();
				mot.moveTo(targetPos);
				Serial.print(F("Target: "));
				Serial.println(String(targetPos));
			}
			else if (key == F("move"))
			{
				if (value == F("forward"))
				{
					mot.moveTo(mot.forwardLimit);
					Serial.println(F("Moving forward"));
				}
				else if (value == F("backward"))
				{
					mot.moveTo(mot.reverseLimit);
					Serial.println(F("Moving backward"));
				}
				else
				{
					Serial.print(F("Unrecognized direction:"));
					Serial.println(value);
				}
			}
			else if (key == F("stop"))
			{
				mot.stop();
				Serial.println(F("Motor stopped"));
			}
			else if (key == F("decelerate"))
			{
				mot.decelerate();
				Serial.println(F("Motor decelerating"));
			}
			else if (key == F("getmaxspeed"))
			{
				Serial.print(F("Max Speed: "));
				Serial.print(String(mot.getMaxSpeed()));
				Serial.println(F(" st/s"));
			}
			else if (key == F("setmaxspeed"))
			{
				word newSpeed = value.toInt();
				Serial.print(F("Max Speed: "));
				Serial.print(String(newSpeed));
				Serial.println(F(" st/s"));
				mot.setMaxSpeed(newSpeed);
			}
			else if (key == F("getcurspeed"))
			{
				Serial.print(F("Current Speed: "));
				Serial.print(String(mot.getCurSpeed()));
				Serial.println(F(" st/s"));
			}
			else if (key == F("getaccel"))
			{
				Serial.print(F("Accel: "));
				Serial.print(String(mot.getAccel()));
				Serial.println(F(" st/s^s"));
			}
			else if (key == F("setaccel"))
			{
				word newAccel = value.toInt();
				bool result = mot.setAccel(newAccel);
				if (result) {
					Serial.print(F("Accel: "));
					Serial.print(String(newAccel));
					Serial.println(F(" st/s^s"));
				}
				else {
					Serial.println(F("Unable to change acceleration while accelerating"));
				}
				
			}
			else if (key == F("getmode"))
			{
				Serial.print(F("Mode: 1/"));
				Serial.print(String(1 << mot.getDriveMode()));
				Serial.println(F(" step"));
			}
			else if (key == F("setmode"))
			{
				Serial.print(F("Mode: "));
				if (value == "1")
				{
					Serial.println(F("full step"));
					mot.setDriveMode(FULL_STEP);
				}
				else if (value == "2")
				{
					Serial.println(F("1/2 step"));
					mot.setDriveMode(HALF_STEP);
				}
				else if (value == "4")
				{
					Serial.println(F("1/4 step"));
					mot.setDriveMode(MICROSTEP_4);
				}
				else if (value == "8")
				{
					Serial.println(F("1/8 step"));
					mot.setDriveMode(MICROSTEP_8);
				}
			}
			else if (key == F("isenabled"))
			{
				if (mot.isEnabled()) Serial.println(F("TRUE"));
				else Serial.println(F("FALSE"));
			}
			else if (key == F("enable"))
			{
				mot.enable();
				Serial.println(F("Motor enabled"));
			}
			else if (key == F("disable"))
			{
				mot.disable();
				Serial.println(F("Motor disabled"));
			}
			else
			{
				Serial.print(F("Unrecognized command:"));
				Serial.println(key);
			}
		}
		else if (commandIn) command += c;
	}
}

void setup(void)
{

	// initialize the kissStepper
	// use 1/8th drive mode, and set the maximum speed to 200 st/s
	mot.begin(MICROSTEP_8, 200); 

	Serial.begin(9600);
	
	// send instructions to serial
	Serial.println("");
	Serial.println(F("Usage:"));
	Serial.println(F("<posunit>           returns the units of measurement for the position index"));
	Serial.println(F("<getpos>            returns the current motor position"));
	Serial.println(F("<target>            returns the target position"));
	Serial.println(F("<ismoving>          returns true if the motor is currently moving"));
	Serial.println(F("<goto x>            sends the motor to position x, -2147483648 <= x <= 2147483647"));
	Serial.println(F("<move forward>      continuously move the motor fowards"));
	Serial.println(F("<move backward>     continuously move the motor backwards"));
	Serial.println(F("<stop>              stops the motor suddenly"));
	Serial.println(F("<decelerate>        if acceleration is on, decelerates the motor to a stop"));
	Serial.println(F("<getmaxspeed>       returns the maximum speed of the motor (in st/s)"));
	Serial.println(F("<setmaxspeed x>     sets the maximum speed of the motor (in st/s), x <= 65535"));
	Serial.println(F("<getcurspeed>       returns the current speed of the motor (in st/s)"));
	Serial.println(F("<getaccel>          returns the acceleration (in st/s^2)"));
	Serial.println(F("<setaccel x>        sets the acceleration (in st/s^2), x <= 65535"));
	Serial.println(F("<getmode>           returns the drive mode of the motor"));
	Serial.println(F("<setmode x>         sets the motor's drive mode, 1 = full step, 2 = 1/2 step, 4 = 1/4 step, 8 = 1/8 step"));
	Serial.println(F("<isenabled>         returns true if the motor is enabled"));
	Serial.println(F("<enable>            enables the motor"));
	Serial.println(F("<disable>           disables the motor"));
	Serial.println("");

}
