**PLEASE NOTE THAT THIS DOCUMENTATION IS NOW VERY OUT OF DATE. NEW DOCUMENTATION IS BEING DRAFTED.**

# kissStepper API documentation for Arduino
For a fully working example that implements most of the API methods below, please see the "SerialControl" sketch in the examples folder.

The simplest way to use kissStepper is as follows:

1. Create a new instance in the global scope
2. Call [*begin()*](#begin) with your chosen settings in your sketch's setup() routine
3. Place [*work()*](#work) in your loop() routine where it will be repeatedly called at regular intervals
4. You can then use [*moveTo()*](#moveto), [*getPos()*](#getpos), and [*stop()*](#stop) to move the motor where you want, stop it where you want, and save its current position for later use (such as returning to certain positions).

## Table of Contents
* [Configuring and Instantiating kissStepper](#configinstance)
	* [kissPinAssignments](#kissPinAssignments)
	* [kissMicrostepConfig](#kissMicrostepConfig)
		* [Determining values for MSxConfig parameters](#MSxConfig)
	* [Instantiating kissStepper](#instantiate)
* [begin()](#begin)
* [work()](#work)
* [Motor Positioning](#positioning)
	* [What the Position Means](#posmeaning)
		* [fullStepVal](#fullStepVal)
	* [moveTo()](#moveto)
	* [getTarget()](#gettarget)
	* [stop()](#stop)
	* [getPos()](#getpos)
	* [setPos()](#setpos)
	* [forwardLimit and reverseLimit](#limits)
	* [getMoveState()](#getMoveState)
* [Motor Speed](#speed)
	* [setMaxSpeed()](#setMaxSpeed)
	* [getMaxSpeed()](#getMaxSpeed)
	* [getCurSpeed()](#getCurSpeed)
* [Drive modes](#drivemodes)
	* [drivemode_t](#drivemode)
	* [setDriveMode()](#setDriveMode)
	* [getDriveMode()](#getDriveMode)
* [Acceleration](#accel)
	* [setAccel()](#setAccel)
	* [getAccel()](#getAccel)
	* [decelerate()](#decelerate)
	* [getAccelState()](#getAccelState)
* [Enabling and Disabling the Controller](#enabledisable)
	* [enable()](#enable)
	* [disable()](#disable)
	* [isEnabled()](#isEnabled)

## <a name="configinstance">Configuring and Instantiating kissStepper</a>
You must create an instance of the kissStepper class before you can interact with your motor. You can create multiple instances to drive multiple motors. This is typically done in the global scope (near the top of your sketch, outside of the loop() or setup() routines).

This is where you set most of the configuration options that tell kissStepper about your motor and motor driver. To make these settings more intuitive, they are organized into two main data structures: [*kissPinAssignments*](#kissPinAssignments) and [*kissMicrostepConfig*](#kissMicrostepConfig).

---
### <a name="kissPinAssignments">kissPinAssignments</a>
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

You specify the pin assignments by creating an instance of the *kissPinAssignments* struct and passing the pin numbers as arguments to the constructor. The pin assignments are specified in this order: DIR, STEP, ENABLE, MS1, MS2, MS3.

If you want to specify pins for MS1, MS2, or MS3, but not ENABLE, use 255 for ENABLE.

#### Example:
```C++
kissPinAssignments pinAssignments(2, 3); // Bare Minimum, 2=DIR, 3=STEP
kissPinAssignments pinAssignments(2, 3, 4); // 2=DIR, 3=STEP, 4=ENABLE
kissPinAssignments pinAssignments(2, 3, 4, 5, 6); // Easy Driver, 2=DIR, 3=STEP, 4=ENABLE, 5=MS1, 6=MS2
kissPinAssignments pinAssignments(2, 3, 4, 5, 6, 7); // Big Easy Driver, 2=DIR, 3=STEP, 4=ENABLE, 5=MS1, 6=MS2, 7=MS3
kissPinAssignments pinAssignments(2, 3, 255, 4, 5, 6); // No enable pin
```

---
### <a name="kissMicrostepConfig">kissMicrostepConfig</a>
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

If you are using an Easy Driver, Big Easy Driver, Allegro A3967, A4983, A4988, or any other Allegro chip with the same microstepping pin configuration, all you need to set here is [*maxMicrostepMode*](#drivemode) (which tells kissStepper the maximum microstepping mode your motor driver is capable of). The [*MSxConfig*](#MSxConfig) values are already set up to correctly operate these motor drivers.

If you are using a different motor driver that uses a microstep pin configuration that differs from the A3967, A4983, or A4988, and still want to automatically control these pins, you need to add the appropriate [*MSxConfig*](#MSxConfig) arguments.

#### Example:
```C++
kissMicrostepConfig microstepConfig(MICROSTEP_8); // For the Easy Driver
kissMicrostepConfig microstepConfig(MICROSTEP_16); // For the Big Easy Driver
kissMicrostepConfig microstepConfig(MICROSTEP_32, 84, 48, 12); // For the TI/Pololu DRV8825
```

#### <a name="MSxConfig">Determining values for MSxConfig parameters</a>
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

And, if you look in the .h file of the library, you'll see that the default values for *MS1Config*, *MS2Config*, and *MS3Config* are, indeed, 88, 56, and 8, respectively.

With the ability to override these three parameters, you can operate a wide variety of STEP/DIR type motor drivers.

---
### <a name="instantiate">Instantiating kissStepper</a>
Once you have set up the configuration structures, you can create an instance of the kissStepper class. Its constructor takes two arguments: *pinAssignments*, and *microstepConfig*. These are the configuration structures described above.

#### Example:
```C++
kissStepper(kissPinAssignments pinAssignments, kissMicrostepConfig microstepConfig); // general syntax
kissStepper myStepper(pinAssignments, microstepConfig);
```

If you don't need to re-use the configuration structures, you can instantiate them directly in the arguments to the kissStepper constructor. This will save a little bit of memory and make your sketch slightly smaller:

#### Example:
```C++
// for the Easy Driver
kissStepper myStepper(
	kissPinAssignments(2, 3, 4, 5, 6),
	kissMicrostepConfig(MICROSTEP_8)
	);
```

## <a name="begin">void begin(driveMode_t mode, uint16_t maxStepsPerSec, uint16_t accelStepsPerSecPerSec): Initializing the kissStepper</a>
You must do this (once) after instantiating the kissStepper. Initializes pin states and member variables. Typically done in the setup() routine.

### Example:
```C++
void setup(void)
{
	...
	myStepper.begin(mode, maxStepsPerSec, accelStepsPerSecPerSec); // general syntax
	myStepperTwo.begin(MICROSTEP_8, 200, 200); // use 1/8th stepping, 200 st/sec, 200 st/sec/sec acceleration
    myStepperThree.begin(); // Bare minimum. Drive mode, speed, and acceleration will be set to defaults.
	...
}
```

### Parameters:
* mode (*optional*): a named constant from [*drivemode_t*](#drivemode), tells the library what drive mode to use.  You should still provide the correct drive mode here, even if you are not using kissStepper to control the microstep/drive mode select pins. Failure to do so will result in incorrect timing of steps. *Defaults to FULL_STEP.*
* maxStepsPerSec (*optional*): the maximum speed, in full steps per second, that the motor will turn at. *Defaults to 100*.
* accelStepsPerSecPerSec (*optional*): [acceleration](#accel), chosen in steps/sec<sup>2</sup>. *Defaults to 0 (off)*.

## <a name="work">bool work(void)</a>
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

## <a name="positioning">Motor Positioning</a>
The following methods and variables allow you to make the motor move, figure out where it is, and limit its motion to a range of positions.

---
### <a name="posmeaning">What the Position Means</a>
It is helpful to understand what the unit of measurement for a position is. For example, if [*getPos()*](#getpos) returns the number 1600, how far has the motor actually moved?

A position value is always measured by the maximum resolution microstep mode that you set when instantiating kissStepper. So, if you set [*maxMicrostepMode*](#kissMicrostepConfig) to *MICROSTEP_8*, then a position value of 1600 corresponds to 1600 1/8th steps or 200 full steps. This will always be true for a given value of [*maxMicrostepMode*](#kissMicrostepConfig), regardless of the current drive mode. This means that you can freely change the drive mode, even when the motor is moving, and the position index and limits will still represent the same physical motor positions.

To facilitate distance calculations in your sketch, a constant is available called [*fullStepVal*](#fullStepVal) whose value is determined during instantiation of kissStepper based on your choice of [*maxMicrostepMode*](#kissMicrostepConfig).

#### <a name="fullStepVal">const uint8_t fullStepVal</a>
This constant tells you the position increment which corresponds to one full step. Its value depends on the maximum drive mode you specified when instantiating kissStepper.

Dividing the value of [*getPos()*](#getpos) by *fullStepVal* will give you the number of full steps the motor has moved from its initial (zero) position.

##### Example:
```C++
int32_t curPos = myStepper.getPos();
int32_t fullStepPos = curPos / myStepper.fullStepVal; // calculate the position in full steps
```

---
### <a name="moveto">bool moveTo(int32_t newTarget)</a>
An essential method that tells the motor to move to a particular location. You only need to call this once for every position - the work() method will then move the motor until the destination is reached. Note that *newTarget* will be constrained to within [*forwardLimit* and *reverseLimit*](#limits). If you don't want the motor to stop at a predetermined end point, but rather just want to move the motor forwards or backwards until stopped with [*stop()*](#stop) or [*decelerate()*](#decelerate), pass the [*forwardLimit* or *reverseLimit*](#limits) member as the parameter.

#### Example:
```C++
myStepper.moveTo(3200); // move to position 3200
myStepper.moveTo(myStepper.forwardLimit); // move forward
myStepper.moveTo(myStepper.reverseLimit); // move backward
```

#### Parameters:
* newTarget (**required**): the target position. Values greater than the current position will make the motor move "forward", while values less than the current position will make the motor move "backward". **If you use this method when the motor is already moving, it will be ignored.**

#### Returns:
True if the command was accepted, otherwise false.

---
### <a name="gettarget">int32_t getTarget(void)</a>

#### Example:
```C++
long targetPos = myStepper.getTarget();
```

#### Returns:
The current target position.

---
### <a name="stop">void stop(void)</a>
Tells the motor to stop. The motor will stop very suddenly, even if you are using acceleration. At high speeds or with heavy loads, the lack of deceleration may cause momentum to carry the motor forward, throwing off your motor's indexing. See [*decelerate()*](#decelerate) for an alternative.

#### Example:
```C++
myStepper.stop();
```

---
### <a name="getpos">int32_t getPos(void)</a>
Gets the current motor position.

#### Example:
```C++
long curPos = myStepper.getPos();
```

#### Returns:
The current position of the motor.

---
### <a name="setpos">void setPos(int32_t newPos)</a>
This changes the current motor position value. It will not cause the motor to move, and can only be called when the motor is stopped. The purpose of this method is to allow you to do things such as calibration of a motorized device, or resetting the position index when desired. The *newPos* parameter will be constrained between [*forwardLimit* and *reverseLimit*](#limits).

#### Example:
```C++
myStepper.setPos(0); // zero the current location
```

#### Parameters:
* newPos (**required**): the new value that will be assigned to the current position.

---
### <a name="limits">int32\_t forwardLimit and int32\_t reverseLimit</a>
These variables hold the maximum forward and reverse position indexes beyond which the motor will not be allowed to move. You can get or change their value however you wish. This allows you to calibrate motorized equipment so that it will not move beyond a given range of positions. Note that if changed while the motor is moving, the motor must be stopped and [*moveTo()*](#moveto) issued again before the changes will take effect.

#### Example:
```C++
// and zero the current position
mot.setPos(0);
// don't let the motor move more than 200 full steps in either direction
uint16_t distLimit = myStepper.fullStepVal * 200
mot.forwardLimit = distLimit;
mot.reverseLimit = -distLimit;
```

---
### <a name="getMoveState">int8_t getMoveState(void)</a>
#### Example:
```C++
int8_t moveState = myStepper.getMoveState();
```

#### Returns:
* 1 if the motor is moving forwards
* 0 if the motor is not moving
* -1 if the motor is moving backwards

## <a name="speed">Motor Speed</a>
The speed of the motor is expressed in steps/sec. Note that accurate speed requires that the drive mode settings are configured correctly.

---
### <a name="setMaxSpeed">void setMaxSpeed(uint16_t stepsPerSec)</a>
Allows changing of the maximum speed after [*begin()*](#begin) is called. This can be done at any time, even while the motor is in motion. If not using acceleration, a moving motor will suddenly change to the new speed. If using acceleration, a moving motor will accelerate or decelerate to the new speed.

#### Example:
```C++
myStepper.setMaxSpeed(200);
```

#### Parameters:
* stepsPerSec (**required**): the new maximum speed

---
### <a name="getMaxSpeed">uint16_t getMaxSpeed(void)</a>
Returns the current maximum speed (steps/sec).

#### Example:
```C++
unsigned int maxSpeed = myStepper.getMaxSpeed();
```
#### Returns:
The set maximum speed in full steps/sec.

---
### <a name="getCurSpeed">uint16_t getCurSpeed(void)</a>
Returns the current speed (steps/sec). Due to acceleration and deceleration, the current speed is often different from the maximum speed. If the motor is not moving, the current speed will be 0.

#### Example:
```C++
unsigned int curSpeed = myStepper.getCurSpeed();
```
#### Returns:
The current speed in steps/sec.

## <a name="drivemodes">Drive Modes</a>
Your motor is capable of moving with greater precision than full steps by a technique called "microstepping". Most stepper motor controllers support some degree of microstepping. More precise microstepping modes reduce vibrations and allow for finer positioning. The different modes, including full stepping, half stepping, and microstepping, and referred to in this library as "drive modes".

---
### <a name="drivemode">enum drivemode_t</a>
This enum contains several named constants which represent various motor drive modes. It is used both as a parameter and as a return value for a number of methods that set/get the drive mode or which configure the maximum allowed drive mode.

The possible values are:

* FULL_STEP for full stepping
* HALF_STEP for half stepping
* MICROSTEP_4 for 1/4 stepping
* MICROSTEP_8 for 1/8 stepping
* MICROSTEP_16 for 1/16 stepping
* MICROSTEP_32 for 1/32 stepping
* MICROSTEP_64 for 1/64 stepping
* MICROSTEP_128 for 1/128 stepping

When working with enums, it is common practice to use them with a switch statement. However, each of these constants also has a numeric uint8_t (byte) value that can be used in calculations. For example, if you wish to write out the current drive mode to Serial or to a display, you can do something like this:

```C++
Serial.print("Current drive mode is: 1/");
Serial.print(String(1 << mot.getDriveMode()));
Serial.println(" step");
```

The numeric value of *FULL_STEP* is 0, and the value of each subsequent drive mode is incremented by 1.

---
### <a name="setDriveMode">void setDriveMode(driveMode_t mode)</a>
Allows changing of the drive mode after [*begin()*](#begin) is called. If you specify a microstepping mode beyond the maximum set in [*kissMicrostepConfig*](#kissMicrostepConfig), the maximum will be used instead. The drive mode can be changed at any time, even while the motor is in motion.

#### Example:
```C++
myStepper.setDriveMode(MICROSTEP_8);
```

#### Parameters:
* mode (**required**): a named constant from [driveMode_t](#drivemode)

---
### <a name="getDriveMode">driveMode_T getDriveMode(void)</a>
Returns a named constant corresponding to the current drive mode.

#### Example:
```C++
kissStepper::driveMode_T driveMode = myStepper.getDriveMode();
```

#### Returns:
a [driveMode_t](#drivemode) named constant

## <a name="accel">Acceleration</a>
When not using acceleration, your motor starts and stops very suddenly. In many cases, such as when driving light loads and moving at low speeds, this is not a problem. However, if you find that your motor is skipping steps or stalling, it may be because it is struggling to start and stop heavy loads or to change speeds more quickly than it is able. Using acceleration can prevent these issues.

---
### <a name="setAccel">void setAccel(uint16_t stepsPerSecPerSec)</a>
Allows changing the rate of acceleration/deceleration after [*begin()*](#begin) is called.

#### Example:
```C++
myStepper.setAccel(200); // accelerate at 200 steps/sec/sec
```

#### Parameters:
* accelStepsPerSecPerSec (**required**): the new rate of acceleration, in steps/sec<sup>2</sup>

---
### <a name="getAccel">uint16_t getAccel(void)</a>
Returns the acceleration rate in steps/sec<sup>2</sup>

#### Example:
```C++
unsigned int accel = myStepper.getAccel();
```

#### Returns:
The acceleration rate in steps/sec<sup>2</sup>

---
### <a name="decelerate">void decelerate(void)</a>
Gradually decelerates the motor down to 0 steps/sec<sup>2</sup> if you have set a non-zero acceleration rate in [*begin()*](#begin) or [*setAccel()*](#setAccel). Otherwise, it will behave very much like stop().

#### Example:
```C++
myStepper.decelerate();
```

---
### <a name="getAccelState">int8_t getAccelState(void)</a>
Returns a value that indicates whether the motor is accelerating, decelerating, or has a constant speed.

#### Example:
```C++
int8_t accelState = myStepper.getAccelState();
```

#### Returns:
* 1 if the motor is accelerating
* 0 if the motor has a constant speed
* -1 if the motor is decelerating 

## <a name="enabledisable">Enabling and Disabling the Controller</a>
If you have supplied kissStepper with an ENABLE pin in [*kissPinAssignments*](#kissPinAssignments), these methods can be used to enable or disable the stepper motor controller.

---
### <a name="enable">void enable(void)</a>
Enables the motor controller. Seldom needed, since kissStepper automatically enables the controller as needed.

#### Example:
```C++
myStepper.enable();
```

---
### <a name="disable">void disable(void)</a>
Disables the motor controller. Can be useful for saving power or minimizing heat when the motor is not actively being used. The motor controller will automatically turn itself on again if asked to move. When the motor controller is disabled, the motor may re-seat itself (move out of place) slightly, especially if using microstepping with a low friction load.

#### Example:
```C++
myStepper.disable();
```

---
### <a name="isEnabled">bool isEnabled(void)</a>
#### Example:
```C++
bool enabled = myStepper.isEnabled();
```

#### Returns:
True if the motor controller is enabled, otherwise false.
