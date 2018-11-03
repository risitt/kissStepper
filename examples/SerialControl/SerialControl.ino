/*

Control an Easy Driver through a serial connection

Written by Rylee Isitt
September 30, 2018

The code in this file is released into the public domain.
Libraries are licensed separately (see their licenses for details).

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

void menu(void)
{
    // send instructions to serial
    Serial.println("");
    Serial.println(F("Usage:"));
    Serial.println(F("<posunit>           returns the units of measurement for the position index"));
    Serial.println(F("<getpos>            returns the current motor position"));
    Serial.println(F("<target>            returns the target position"));
    Serial.println(F("<state>             returns the motor's state"));
    Serial.println(F("<goto x>            sends the motor to position x, -2147483648 <= x <= 2147483647"));
    Serial.println(F("<rev forward>       moves the motor forward by one revolution"));
    Serial.println(F("<rev backward>      moves the motor backward by one revolution"));
    Serial.println(F("<move forward>      continuously move the motor fowards"));
    Serial.println(F("<move backward>     continuously move the motor backwards"));
    Serial.println(F("<stop>              stops the motor suddenly"));
    Serial.println(F("<decelerate>        if acceleration is on, decelerates the motor to a stop"));
    Serial.println(F("<getmaxspeed>       returns the maximum speed of step pin pulses (in Hz)"));
    Serial.println(F("<setmaxspeed x>     sets the maximum speed of step pin pulses (in Hz), x <= 65535"));
    Serial.println(F("<getcurspeed>       returns the current speed of step pin pulses (in Hz)"));
    Serial.println(F("<getaccel>          returns the acceleration (in Hz/s)"));
    Serial.println(F("<setaccel x>        sets the acceleration (in Hz/s), x <= 65535"));
    Serial.println(F("<isenabled>         returns true if the motor is enabled"));
    Serial.println(F("<enable>            enables the motor"));
    Serial.println(F("<disable>           disables the motor"));
    Serial.println(F("<menu>              repeat this menu"));
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

    // static vars
    static String command = "";
    static bool commandIn = false;

    // this line below actually makes the motor move
    // and needs to be run repeatedly and often for smooth motion
    // you can do things this way, or you can put move() in a for/while loop
    // or you can call move() from a timer interrupt
    mot.move();

    // check for serial data
    // get commands from serial
    while (Serial.available() > 0)
    {
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

            Serial.print(F("\n"));
            if (key == F("posunit"))
            {
                Serial.print(F("The position is measured in 1/"));
                Serial.print(DRIVE_MODE);
                Serial.println(F(" steps"));
            }
            else if (key == F("getpos"))
            {
                int32_t motorPos = mot.getPos();
                Serial.print(F("Microsteps: "));
                Serial.println(String(motorPos));
                Serial.print(F("Steps: "));
                Serial.println(String((float)motorPos / DRIVE_MODE));
                Serial.print(F("Revolutions: "));
                Serial.println(String((float)motorPos / REVOLUTION_PULSES));
            }
            else if (key == F("target"))
            {
                Serial.print(F("Target: "));
                Serial.println(String(mot.getTarget()));
            }
            else if (key == F("state"))
            {
                switch (mot.getState())
                {
                case STATE_STOPPED:
                    Serial.println(F("stopped"));
                    break;
                case STATE_STARTING:
                    Serial.println(F("starting"));
                    break;
                case STATE_RUN:
                    Serial.println(F("constant speed"));
                    break;
                case STATE_ACCEL:
                    Serial.println(F("accelerating"));
                    break;
                case STATE_DECEL:
                    Serial.println(F("decelerating"));
                    break;
                }
            }
            else if (key == F("goto"))
            {
                int32_t targetPos = value.toInt();
                Serial.print(F("Target: "));
                Serial.println(String(targetPos));
                mot.prepareMove(targetPos);
                Serial.print(F("Profile (accel/run/decel): "));
                Serial.print(String(mot.getAccelDist()));
                Serial.print(F("/"));
                Serial.print(String(mot.getRunDist()));
                Serial.print(F("/"));
                Serial.println(String(mot.getDecelDist()));
            }
            else if (key == F("rev"))
            {
                if (value == F("forward"))
                {
                    Serial.println(F("Moving forward"));
                    mot.prepareMove(mot.getPos() + REVOLUTION_PULSES);
                }
                else if (value == F("backward"))
                {
                    Serial.println(F("Moving backward"));
                    mot.prepareMove(mot.getPos() - REVOLUTION_PULSES);
                }
                else
                {
                    Serial.print(F("Unrecognized direction:"));
                    Serial.println(value);
                }
            }
            else if (key == F("move"))
            {
                if (value == F("forward"))
                {
                    Serial.println(F("Moving forward"));
                    mot.prepareMove(mot.getForwardLimit());
                }
                else if (value == F("backward"))
                {
                    Serial.println(F("Moving backward"));
                    mot.prepareMove(mot.getReverseLimit());
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
                Serial.println(F(" Hz"));
            }
            else if (key == F("setmaxspeed"))
            {
                word newSpeed = value.toInt();
                Serial.print(F("Max Speed: "));
                Serial.print(String(newSpeed));
                Serial.println(F(" Hz"));
                mot.setMaxSpeed(newSpeed);
            }
            else if (key == F("getcurspeed"))
            {
                Serial.print(F("Current Speed: "));
                Serial.print(String(mot.getCurSpeed()));
                Serial.println(F(" Hz"));
            }
            else if (key == F("getaccel"))
            {
                Serial.print(F("Accel: "));
                Serial.print(String(mot.getAccel()));
                Serial.println(F(" Hz/s"));
            }
            else if (key == F("setaccel"))
            {
                word newAccel = value.toInt();
                mot.setAccel(newAccel);
                Serial.print(F("Accel: "));
                Serial.print(String(newAccel));
                Serial.println(F(" Hz/s"));
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
            else if (key == F("menu"))
            {
                menuPrinted = false;
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
