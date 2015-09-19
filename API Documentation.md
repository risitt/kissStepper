# kissStepper API documentation for Arduino

For a fully working example that implements most of the API methods below, please see the "SerialControl" sketch in the examples folder.

The simplest way to use kissStepper is as follows:

1. Instantiate a new instance in the global scope
2. Call begin() with your chosen settings in your sketch's setup() routine
3. Place work() in your loop() routine where it will be repeatedly called at regular intervals
4. You can then use moveTo(), getPos(), and stop() to move the motor where you want, stop it where you want, and save its current position for later use (such as returning to certain positions).

## Creating a new instance

You must create an instance of the kissStepper class before you can interact with your motor. You can create multiple instances to drive multiple motors. This is typically done in the global scope (near the top of your sketch, outside of the loop() or setup() routines).
### Example:
```C++
kissStepper myStepper(pinEnable, pinDir, pinStep, pinMS1, pinMS2, pinMS3); // general syntax
kissStepper myStepperTwo(7, 3, 4, 5, 6) // Easy Driver with software control of enable, MS1, and MS2 pins
kissStepper myStepperThree(255, 8, 9) // Bare minimum - only two pins used
```


### Parameters:
* pinEnable (**set to 255 if not used**): Allows kissStepper to automatically enable the motor driver as needed and allows you to enable or disable it using the enable() or disable() methods.
* pinDir (**required**): Set this to the pin number for the DIR (direction) pin
* pinStep (**required**): Set this to the pin number for the STEP pin
* pinMS1...pinMS3 (*optional*): If you would like to be able to control the stepper motor's drive mode with your microcontroller, pass the relevant pin numbers here. Allegro chips have between 1 and 3  MS pins, depending on the step resolution. Only pass the ones you are using.

## void begin(uint16_t motorSteps, driveMode_t mode, uint16_t maxRPM, uint16_t accel): Initializing the kissStepper

Do this after instantiating the kissStepper. Initializes pin states and member variables. Typically done in the setup() routine.
### Example:
```C++
void setup(void)
{
	...
	myStepper.begin(motorSteps, driveMode, maxRPM, accel); // general syntax
	myStepperTwo.begin(200, kissStepper::EIGHTH, 60, 3000); // motor has 200 st/rev, use 1/8th stepping, 60 RPM, 3000 RPMPM acceleration
    myStepperThree.begin(200); // Bare minimum - drive mode, speed, and acceleration will be set to defaults.
	...
}
```

### Parameters:
* motorSteps (**required**): The number of full steps per one revolution of your motor. For 3.6 degree/step motors, it is 100. For 1.8 degree/step motors, it is 200.
* mode (*optional*): even if you are not controlling the MS1...MS3 pins with your microcontroller, it is useful to still pass the correct drive mode here if you want accurate speed and acceleration.
	* kissStepper::FULL to specify full stepping (*default*)
	* kissStepper::HALF for half stepping
	* kissStepper::QUARTER for 1/4 stepping
	* kissStepper::EIGHTH for 1/8 stepping
	* kissStepper::SIXTEENTH for 1/16 stepping
* maxRPM (*optional*): the maximum speed, in RPM, that the motor will turn at. *Defaults to 30*.
* accel (*optional*): acceleration, chosen in RPM per minute (RPMPM). Acceleration can help to prevent skipped steps or motor stalling when driving heavy loads or moving at high speed. *Defaults to 0 (off)*.

## bool work(void)

This method does most of the heavy lifting. **You must call it repeatedly and often**. A good place to do this is inside the loop() routine. You can also use it in while loops, or call it at regular intervals using a timer interrupt.
### Example:
```C++
void loop(void)
{
	...
    bool isMoving = myStepper.work();
	...
}
```

### Returns:
True if the motor is moving, otherwise false.

## bool moveTo(int32_t newTarget)

A very useful method that tells the motor to move to a particular location. You only need to call this once - the work() method will then move the motor until the destination is reached.
### Example:
```C++
myStepper.moveTo(3200);
```

### Parameters:
* newTarget (**required**): the target position, measured in 1/16th steps. For a motor with 200 full steps per revolution, position 3200 is one revolution forward from the starting position. If at 0, positive values will make the motor move "forward", while negative values will make the motor move "backward". **If you use this method when the motor is already moving, it will be ignored.**

### Returns:
True if the command was accepted, otherwise false.


## bool moveForward(void)
Tells the motor to move forward until either stopped, or the maximum extent (defaults to position 2,147,483,647) is reached. **If you use this method when the motor is already moving, it will be ignored.**
### Example:
```C++
myStepper.moveForward();
```

### Returns:
True if the command was accepted, otherwise false.

## bool moveBackward(void)
Tells the motor to move backward until either stopped, or the maximum extent (defaults to position -2,147,483,648) is reached. **If you use this method when the motor is already moving, it will be ignored.**
### Example:
```C++
myStepper.moveBackward();
```

### Returns:
True if the command was accepted, otherwise false.

## void stop(void)

Tells the motor to stop. If using acceleration, it will not stop right away, but will decelerate at the chosen rate, continuining to move for some distance.
### Example:
```C++
myStepper.stop();
```

## void hardStop(void)

Like stop(), but will suddenly stop the motor even if acceleration is being used. At high speeds or with heavy loads, the lack of deceleration may cause momentum to carry the motor forward, throwing off your motor's indexing.
### Example:
```C++
myStepper.hardStop();
```

## int32_t getPos(void)

Gets the current motor position.
### Example:
```C++
long curPos = myStepper.getPos();
```

### Returns:
The current position of the motor measured in 1/16th steps.

## void setDriveMode(driveMode_t mode)

Allows changing of the drive mode after begin() is called. The drive mode can be changed at any time, even while the motor is in motion.

### Example:
```C++
myStepper.setDriveMode(kissStepper::EIGHTH);
```

### Parameters:
* driveMode (**required**)
	* kissStepper::FULL to specify full stepping
	* kissStepper::HALF for half stepping
	* kissStepper::QUARTER for 1/4 stepping
	* kissStepper::EIGHTH for 1/8 stepping
	* kissStepper::SIXTEENTH for 1/16 stepping

## driveMode_T getDriveMode(void)
Returns an enum value corresponding to the current drive mode.
### Example:
```C++
kissStepper::driveMode_T driveMode = myStepper.getDriveMode();
```

### Returns:
* kissStepper::FULL for full step mode
* kissStepper::HALF for half step mode
* kissStepper::QUARTER for 1/4 step mode
* kissStepper::EIGHTH for 1/8 step mode
* kissStepper::SIXTEENTH for 1/16 step mode

## void setMaxRPM(uint16_t maxRPM)
Allows changing of the maximum RPM after begin() is called. This can be done at any time, even while the motor is in motion.

### Example:
```C++
myStepper.setMaxRPM(120);
```

### Parameters:
* maxRPM (**required**): the new maximum speed

## uint16_t getMaxRPM(void)
Returns the current maximum speed (RPM).
### Example:
```C++
unsigned int maxSpeed = myStepper.getMaxRPM();
```
### Returns:
The set maximum revolutions per minute.

## uint16_t getCurRPM(void)
Returns the current speed (RPM). Due to acceleration and deceleration, the current speed is often less than the maximum speed. If the motor is not moving, the current speed will be 0.
### Example:
```C++
unsigned int curSpeed = myStepper.getCurRPM();
```
### Returns:
The current revolutions per minute.

## bool setAccel(uint16_t revPerMinPerMin)

Allows changing the rate of acceleration/deceleration after begin() is called. **This can only be done if the motor is not currently accelerating/decelerating.** 

### Example:
```C++
myStepper.setAccel(1000);
```

### Parameters:
* revPerMinPerMin (**required**): the new rate of acceleration, in RPM per minute

### Returns:
True if the command was accepted, otherwise false.

## uint16_t getAccel(void)

Returns the acceleration rate in RPM per minute

### Example:
```C++
unsigned int accel = myStepper.getAccel();
```

### Returns:
The acceleration rate in revolutions per minute per minute

## void enable(void)

Enables the motor controller. Seldom needed, since the work() method automatically enables the controller when you ask the motor to move.
### Example:
```C++
myStepper.enable();
```

## void disable(void)

Disables the motor controller. Can be useful for saving power or minimizing heat when the motor is not actively being used. The motor controller will automatically turn itself on again if asked to move. When the motor controller is disabled, it may re-seat itself slightly, especially if using microstepping with a low friction load.

### Example:
```C++
myStepper.disable();
```

## bool isEnabled(void)
### Example:
```C++
bool enabled = myStepper.isEnabled();
```

### Returns:
True if the motor controller is enabled, otherwise false.

## int32_t getTarget(void)

### Example:
```C++
long targetPos = myStepper.getTarget();
```

### Returns:
The current target position, measured in 1/16th steps.
