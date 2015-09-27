kissStepper API documentation for Arduino
=========================================
For a fully working example that implements most of the API methods below, please see the "SerialControl" sketch in the examples folder.

The simplest way to use kissStepper is as follows:

1. Create a new instance in the global scope
2. Call begin() with your chosen settings in your sketch's setup() routine
3. Place work() in your loop() routine where it will be repeatedly called at regular intervals
4. You can then use moveTo(), getPos(), and stop() to move the motor where you want, stop it where you want, and save its current position for later use (such as returning to certain positions).

Creating a new instance
-----------------------
You must create an instance of the kissStepper class before you can interact with your motor. You can create multiple instances to drive multiple motors. This is typically done in the global scope (near the top of your sketch, outside of the loop() or setup() routines).

This is where you set most of the configuration options that tell kissStepper about your motor and motor driver. To make these settings more intuitive, they are organized into two main data structures: kissPinAssignments and kissMicrostepConfig.

### kissPinAssignments
```C++
struct kissPinAssignments
{
	const uint8_t pinEnable;
	const uint8_t pinDir;
	const uint8_t pinStep;
	const uint8_t pinMS1;
	const uint8_t pinMS2;
	const uint8_t pinMS3;
};
```

This structure contains all of the pin assignments for your motor driver. At the very least, you need to specify pins for DIR and STEP. You can also specify pins for ENABLE and up to three drive mode/microstep select (MS1...MS3) pins.

You specify the pin assignments by creating an instance of the kissPinAssignments struct and passing the pin numbers as arguments to the constructor. The pin assignments are specified in this order: DIR, STEP, ENABLE, MS1, MS2, MS3.

If you want to specify pins for MS1, MS2, or MS3, but not ENABLE, use 255 for ENABLE.

#### Example:
```C++
kissPinAssignments pinAssignments(2, 3); // Bare Minimum, 2=DIR, 3=STEP
kissPinAssignments pinAssignments(2, 3, 4); // 2=DIR, 3=STEP, 4=ENABLE
kissPinAssignments pinAssignments(2, 3, 4, 5, 6); // Easy Driver, 2=DIR, 3=STEP, 4=ENABLE, 5=MS1, 6=MS2
kissPinAssignments pinAssignments(2, 3, 4, 5, 6, 7); // Big Easy Driver, 2=DIR, 3=STEP, 4=ENABLE, 5=MS1, 6=MS2, 7=MS3
kissPinAssignments pinAssignments(2, 3, 255, 4, 5, 6); // No enable pin
```

### kissMicrostepConfig
```C++
struct kissMicrostepConfig
{	
	const driveMode_t maxMicrostepMode;
	const uint8_t MS1Config;
	const uint8_t MS2Config;
	const uint8_t MS3Config;
};
```

This structure contains settings that tell the kissStepper library about your motor driver's different drive modes and how to access them. Like with *kissPinAssignments*, you must specify the configuration values by passing them to the constructor.

If you are using an Easy Driver, Big Easy Driver, Allegro A3967, A4983, A4988, or any other Allegro chip with the same microstepping pin configuration, all you need to set here is *maxMicrostepMode* (which tells kissStepper the maximum microstepping mode your motor driver is capable of). The MSxConfig values are already set up to correctly operate these motor drivers.

#### Example:
```C++
kissMicrostepConfig microstepConfig(MICROSTEP_8); // For the Easy Driver
kissMicrostepConfig microstepConfig(MICROSTEP_16); // For the Big Easy Driver
```

If you are using a different motor driver that uses a microstep pin configuration that differs from the A3967, A4983, or A4988, you need to add the appropriate MSxConfig arguments.


#### Determining values for MSxConfig parameters
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

And, if you look in the .h file of the library, you'll see that the default values for MS1Config, MS2Config, and MS3Config are, indeed, 88, 56, and 8, respectively.

With the ability to override these three parameters, you can operate a wide variety of STEP/DIR type motor drivers.

##### Example:
```C++
kissMicrostepConfig microstepConfig(MICROSTEP_32, 84, 48, 12); // For the TI/Pololu DRV8825
```

### Instantiating kissStepper

Once you have set up the configuration structures, you can create an instance of the kissStepper class. Its constructor takes three arguments: *motorSteps*, *pinAssignments*, and *microstepConfig*. The first specifies the number of full steps per revolution of your stepper motor. The last two are the configuration structures described above.

### Example:
```C++
kissStepper(uint16_t motorSteps, kissPinAssignments pinAssignments, kissMicrostepConfig microstepConfig); // general syntax
kissStepper myStepper(200, pinAssignments, microstepConfig);
```

If you don't need to re-use the configuration structures, you can instantiate them directly in the arguments to the kissStepper constructor. This will save a little bit of memory and make your sketch slightly smaller:

### Example:
```C++
// for the Easy Driver
kissStepper myStepper(
	200,
	kissPinAssignments(2, 3, 4, 5, 6),
	kissMicrostepConfig(MICROSTEP_8)
	);
```

void begin(driveMode_t mode, uint16_t maxRPM, uint16_t accelRPMS): Initializing the kissStepper
--------------------------------------------------------------------------------------------------------------------

Do this after instantiating the kissStepper. Initializes pin states and member variables. Typically done in the setup() routine.
### Example:
```C++
void setup(void)
{
	...
	myStepper.begin(mode, maxRPM, accelRPMS); // general syntax
	myStepperTwo.begin(MICROSTEP_8, 60, 60); // use 1/8th stepping, 60 RPM, 60 RPM/s acceleration
    myStepperThree.begin(); // Bare minimum - drive mode, speed, and acceleration will be set to defaults.
	...
}
```

### Parameters:
* mode (*optional*): even if you are not controlling the MS1...MS3 pins with your microcontroller, it is useful to still pass the correct drive mode here if you want accurate speed and acceleration.
	* FULL_STEP to specify full stepping (*default*)
	* HALF_STEP for half stepping
	* MICROSTEP_4 for 1/4 stepping
	* MICROSTEP_8 for 1/8 stepping
	* MICROSTEP_16 for 1/16 stepping
	* MICROSTEP_32 for 1/32 stepping
	* MICROSTEP_64 for 1/64 stepping
	* MICROSTEP_128 for 1/128 stepping
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

Tells the motor to stop. The motor will stop very suddenly, even if you are using acceleration. At high speeds or with heavy loads, the lack of deceleration may cause momentum to carry the motor forward, throwing off your motor's indexing.
### Example:
```C++
myStepper.stop();
```

## void decelerate(void)

Gradually decelerates the motor down to 0 RPM at the previously set rate of acceleration. Will only function correctly if you have set an acceleration rate in begin() or setAccel(). Otherwise, it will behave very much like stop().
### Example:
```C++
myStepper.decelerate();
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

Allows changing of the drive mode after begin() is called. If you specify a microstepping mode beyond what you have set as the limit when creating the kissStepper instance, the highest microstep drive mode will be used instead. The drive mode can be changed at any time, even while the motor is in motion.

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
Allows changing of the maximum RPM after begin() is called. This can be done at any time, even while the motor is in motion. If not using acceleration, a moving motor will suddenly change to the new speed. If using acceleration, a moving motor will accelerate or decelerate to the new speed at the previously specified rate.

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
