# kissStepper API documentation for Arduino

For a fully working example that implements most of the API methods below, please see the "SerialControl" sketch in the examples folder.

The simplest way to use kissStepper is as follows:

1. Instantiate a new instance in the global scope
2. Call begin() with your chosen settings in your sketch's setup() routine
3. Place work() in your loop() routine where it will be repeatedly called at regular intervals
4. You can then use moveTo(), getPos(), and stop() to move the motor where you want, stop it where you want, and save its current position for later use (such as returning to certain positions).

## Creating a new instance

You must create an instance of the kissStepper class before you can interact with your motor. You can create multiple instances to drive multiple motors. This is typically done in the global scope (near the top of your sketch, outside of the loop() or setup() routines).

There are different constructors for different feature sets. Everything after pinStep is optional, but may be required to make your motor driver work correctly with this library.

The library is, by default, configured for the Easy Driver, Big Easy Driver, and several Allegro driver chips. However, by passing values to the MS1Config, MS2Config, and MS3Config parameters, you can reconfigure the library for other STEP/DIR drivers with up to 3 microstep select / drive mode setting pins.

### Example:
```C++
kissStepper(uint8_t pinEnable, uint8_t pinDir, uint8_t pinStep, uint8_t pinMS1, uint8_t pinMS2, uint8_t pinMS3, uint8_t MS1Config, uint8_t MS2Config, uint8_t MS3Config); // general syntax
kissStepper myStepper(2, 3, 4, 5, 6, 7, 84, 48, 12); // Full feature set and configuration for the Pololu 8825
kissStepper myStepperTwo(7, 3, 4, 5, 6); // Easy Driver with software control of enable, MS1, and MS2 pins
kissStepper myStepperThree(255, 8, 9); // Bare minimum - only two pins used
```


### Parameters:
* pinEnable (**set to 255 if not used**): Allows kissStepper to automatically enable the motor driver as needed and allows you to enable or disable it using the enable() or disable() methods.
* pinDir (**required**): Set this to the pin number for the DIR (direction) pin
* pinStep (**required**): Set this to the pin number for the STEP pin
* pinMS1...pinMS3 (*optional*): If you would like to be able to control the stepper motor's drive mode with your microcontroller, pass the relevant pin numbers here. Allegro chips have between 1 and 3  MS pins, depending on the step resolution. Only pass the ones you are using.
* MS1Config (*optional*): If provided, overrides the default configuration of drive mode pin states for the first drive mode setting pin.
* MS2Config (*optional*): If provided, overrides the default configuration of drive mode pin states for the second drive mode setting pin.
* MS3Config (*optional*): If provided, overrides the default configuration of drive mode pin states for the third drive mode setting pin.

### Determining values for MSxConfig parameters
These three 8-bit parameters contain the states of the drive mode select pins for all drive modes (full to 1/128 stepping).

The most significant bit (left-most) is the pin state for full stepping, and the least significant bit (right-most) is the pin state for 1/128 stepping.

For example, most Allegro chips follow this pattern:

    MODE    MS1    MS2    MS3    MSxConfig BIT#
    FULL    0      0      0      7
    HALF    1      0      0      6
    1/4     0      1      0      5
    1/8     1      1      0      4
    1/16    1      1      1      3
    1/32    n/a    n/a    n/a    2
    1/64    n/a    n/a    n/a    1
    1/128   n/a    n/a    n/a    0

So, for MS1, we get 01011000, which turns out to be the number 88, MS2 is 00111000 or 56, and MS3 is 00001000 or 8.

And, if you look in the .cpp file of the library, you'll see that the default values for MS1Config, MS2Config, and MS3Config are, indeed, 88, 56, and 8, respectively.

With the ability to override these three parameters, you can make the kissStepper library compatible with most STEP/DIR motor drivers that support up to 1/128 microstepping.




## void begin(uint16_t motorSteps, driveMode_t mode, uint16_t maxRPM, uint16_t accelRPMS): Initializing the kissStepper

Do this after instantiating the kissStepper. Initializes pin states and member variables. Typically done in the setup() routine.
### Example:
```C++
void setup(void)
{
	...
	myStepper.begin(motorSteps, mode, maxRPM, accelRPMS); // general syntax
	myStepperTwo.begin(200, kissStepper::MICRO8, 60, 60); // motor has 200 st/rev, use 1/8th stepping, 60 RPM, 60 RPM/s acceleration
    myStepperThree.begin(200); // Bare minimum - drive mode, speed, and acceleration will be set to defaults.
	...
}
```

### Parameters:
* motorSteps (**required**): The number of full steps per one revolution of your motor. For 3.6 degree/step motors, it is 100. For 1.8 degree/step motors, it is 200.
* mode (*optional*): even if you are not controlling the MS1...MS3 pins with your microcontroller, it is useful to still pass the correct drive mode here if you want accurate speed and acceleration.
	* kissStepper::FULL to specify full stepping (*default*)
	* kissStepper::HALF for half stepping
	* kissStepper::MICRO4 for 1/4 stepping
	* kissStepper::MICRO8 for 1/8 stepping
	* kissStepper::MICRO16 for 1/16 stepping
	* kissStepper::MICRO32 for 1/32 stepping
	* kissStepper::MICRO64 for 1/64 stepping
	* kissStepper::MICRO128 for 1/128 stepping
* maxRPM (*optional*): the maximum speed, in RPM, that the motor will turn at. *Defaults to 30*.
* accelRPMS (*optional*): acceleration, chosen in RPM/s. Acceleration can help to prevent skipped steps or motor stalling when driving heavy loads or moving at high speed. *Defaults to 0 (off)*.

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

An essential method that tells the motor to move to a particular location. You only need to call this once for every position - the work() method will then move the motor until the destination is reached. Note that newTarget will be constrained to within forwardLimit and reverseLimit. If you don't want the motor to stop at a predetermined end point, but rather just want to move the motor forwards or backwards until stopped with stop() or hardStop(), pass the forwardLimit or reverseLimit member as the parameter.
### Example:
```C++
myStepper.moveTo(3200); // move to position 3200
myStepper.moveTo(myStepper.forwardLimit); // move forward
myStepper.moveTo(myStepper.reverseLimit); // move backward
```

### Parameters:
* newTarget (**required**): the target position, measured in 1/128 steps. For a motor with 200 full steps per revolution, position 25600 is one revolution forward from the starting position. If at 0, positive values will make the motor move "forward", while negative values will make the motor move "backward". **If you use this method when the motor is already moving, it will be ignored.**

### Returns:
True if the command was accepted, otherwise false.

## void stop(void)

Tells the motor to stop. If using acceleration, it will not stop right away, but will decelerate at the chosen rate, continuing to move for some distance.
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
## void setPos(int32_t newPos)

This changes the current motor position value. It will not cause the motor to move, and can only be called when the motor is stopped. The purpose of this method is to allow you to do things such as calibration of a motorized device, or resetting the position index when desired. The newPos parameter will be constrained between forwardLimit and reverseLimit.

### Example:
```C++
myStepper.setPos(0); // zero the current location
```

### Parameters:
* newPos (**required**): the new value that will be assigned to the current position, measured in 1/128 steps.

## int32_t getPos(void)

Gets the current motor position.
### Example:
```C++
long curPos = myStepper.getPos();
```

### Returns:
The current position of the motor measured in 1/128 steps.

## int32\_t forwardLimit and int32\_t reverseLimit

These variables hold the maximum forward and reverse position indexes (in 1/128 steps) beyond which the motor will not be allowed to move. You can get or change their value however you wish. This allows you to calibrate motorized equipment so that it will not move beyond a given range of positions. Note that if changed while the motor is moving, the motor must be stopped and moveTo() issued again before the changes will take effect.

### Example:
```C++
// and zero the current position
mot.setPos(0);
// don't let the motor move more than one revolution forward or backward from the current position
mot.forwardLimit = 25600;
mot.reverseLimit = -25600;
```

## void setDriveMode(driveMode_t mode)

Allows changing of the drive mode after begin() is called. The drive mode can be changed at any time, even while the motor is in motion.

### Example:
```C++
myStepper.setDriveMode(kissStepper::MICRO8);
```

### Parameters:
* driveMode (**required**)
	* kissStepper::FULL to specify full stepping
	* kissStepper::HALF for half stepping
	* kissStepper::MICRO4 for 1/4 stepping
	* kissStepper::MICRO8 for 1/8 stepping
	* kissStepper::MICRO16 for 1/16 stepping
	* kissStepper::MICRO32 for 1/32 stepping
	* kissStepper::MICRO64 for 1/64 stepping
	* kissStepper::MICRO128 for 1/128 stepping

## driveMode_T getDriveMode(void)
Returns an enum value corresponding to the current drive mode.
### Example:
```C++
kissStepper::driveMode_T driveMode = myStepper.getDriveMode();
```

### Returns:
* kissStepper::FULL for full stepping
* kissStepper::HALF for half stepping
* kissStepper::MICRO4 for 1/4 stepping
* kissStepper::MICRO8 for 1/8 stepping
* kissStepper::MICRO16 for 1/16 stepping
* kissStepper::MICRO32 for 1/32 stepping
* kissStepper::MICRO64 for 1/64 stepping
* kissStepper::MICRO128 for 1/128 stepping

## void setMaxRPM(uint16_t newMaxRPM)
Allows changing of the maximum RPM after begin() is called. This can be done at any time, even while the motor is in motion.

### Example:
```C++
myStepper.setMaxRPM(120);
```

### Parameters:
* newMaxRPM (**required**): the new maximum speed

## uint16_t getMaxRPM(void)
Returns the current maximum speed (RPM).
### Example:
```C++
unsigned int maxSpeed = myStepper.getMaxRPM();
```
### Returns:
The set maximum revolutions per minute.

## void setMaxRP10M(uint16_t newMaxRP10M)
Like setMaxRPM, but the parameter here is for revolutions per *ten minutes*. This allows setting a more precise speed than using RPM. This can be done at any time, even while the motor is in motion.

### Example:
```C++
myStepper.setMaxRP10M(605); // sets the maximum speed to 60.5 RPM
```

### Parameters:
* newMaxRP10M (**required**): the new maximum speed

## uint16_t getMaxRP10M(void)
Returns the current maximum speed in revolutions per *ten minutes*.
### Example:
```C++
unsigned int maxSpeed = myStepper.getMaxRP10M();
```
### Returns:
The set maximum revolutions per ten minutes.

## uint16_t getCurRPM(void)
Returns the current speed (RPM). Due to acceleration and deceleration, the current speed is often less than the maximum speed. If the motor is not moving, the current speed will be 0.
### Example:
```C++
unsigned int curSpeed = myStepper.getCurRPM();
```
### Returns:
The current revolutions per minute.

## uint16_t getCurRP10M(void)
Returns the current speed in revolutions per *10 minutes* (RP10M). This allows you to get a more precise indication of the motor's current speed by adding a 1/10ths decimal place to RPM (60.5 RPM is returned as 605 RP10M).
### Example:
```C++
unsigned int curSpeed = myStepper.getCurRP10M();
```
### Returns:
The current revolutions per ten minutes.

## bool setAccel(uint16_t RPMS)

Allows changing the rate of acceleration/deceleration after begin() is called. **This can only be done if the motor is not currently accelerating/decelerating.** 

### Example:
```C++
myStepper.setAccel(1000);
```

### Parameters:
* RPMS (**required**): the new rate of acceleration, in RPM/s

### Returns:
True if the command was accepted, otherwise false.

## uint16_t getAccel(void)

Returns the acceleration rate in RPM/s

### Example:
```C++
unsigned int accel = myStepper.getAccel();
```

### Returns:
The acceleration rate in RPM/s

## int8_t getAccelState(void)

Returns a value that indicates whether the motor is accelerating, decelerating, or has a constant speed.

### Example:
```C++
int8_t accelState = myStepper.getAccelState();
```

### Returns:
* 1 if the motor is accelerating
* 0 if the motor has a constant speed
* -1 if the motor is decelerating 


## void enable(void)

Enables the motor controller. Seldom needed, since the work() method automatically enables the controller when you ask the motor to move.
### Example:
```C++
myStepper.enable();
```

## void disable(void)

Disables the motor controller. Can be useful for saving power or minimizing heat when the motor is not actively being used. The motor controller will automatically turn itself on again if asked to move. When the motor controller is disabled, the motor may re-seat itself (move out of place) slightly, especially if using microstepping with a low friction load.

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

## bool isMoving(void)
### Example:
```C++
bool moving = myStepper.isMoving();
```

### Returns:
True if the motor is moving, otherwise false.

## int32_t getTarget(void)

### Example:
```C++
long targetPos = myStepper.getTarget();
```

### Returns:
The current target position, measured in 1/128 steps.
