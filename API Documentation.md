# kissStepper API documentation for Arduino and Teensyduino
For a fully working example that implements most of the API methods below, please see the "SerialControl" sketch in the examples folder.

The simplest way to use kissStepper is as follows:

1. Include kissStepper.h into your sketch
2. Create a new instance in the global scope
3. Call [*begin()*](#void-beginvoid) in your sketch's setup() routine
4. Place [*move()*](#kissstate_t-movevoid) in your loop() routine where it will be repeatedly called at regular intervals
5. You can then use [*setMaxSpeed()*](#void-setmaxspeeduint16_t-maxspeed), [*setAccel()*](#void-setacceluint16_t-accel), [*prepareMove()*](#bool-preparemoveint32_t-target), [*stop()*](#void-stopvoid), [*decelerate()*](#void-deceleratevoid), and [*getPos()*](#int32_t-getposvoid) to set motor speed, move the motor where you want, stop it where you want, and save its current position for later use (such as returning to certain positions)

## Table of Contents
* [Installation](#installation)
    * [Automatic Installation](#automatic-installation)
    * [Manual Installation](#manual-installation)
* [Implementation Notes](#implementation-notes)
    * [Instantiation and Initialization](#instantiation-and-initialization)
    * [Full-stepping/Microstepping](#full-steppingmicrostepping)
    * [Position, Speed, and Acceleration Units of Measurement](#position-speed-and-acceleration-units-of-measurement)
    * [When "Forwards" is Not Forwards](#when-forwards-is-not-forwards)
    * [Disabling Acceleration](#disabling-acceleration)
    * [Driving Multiple Motors](#driving-multiple-motors)
* [Library Reference](#library-reference)
    * [Instantiation and Initialization](#instantiation-and-initialization-1)
        * [The kissStepper Class](#kissstepperuint8_t-pin_dir-uint8_t-pin_step-uint8_t-pin_enable)
        * [The kissStepperNoAccel Class](#kisssteppernoacceluint8_t-pin_dir-uint8_t-pin_step-uint8_t-pin_enable)
        * [begin](#void-beginvoid)
    * [Essential Methods and Types](#essential-methods-and-types)
        * [The kissState_t enum Type](#the-kissstate_t-enum-type)
        * [getPos](#int32_t-getposvoid)
        * [move](#kissstate_t-movevoid)
        * [prepareMove](#bool-preparemoveint32_t-target)
        * [stop](#void-stopvoid)
    * [Working with Speed](#working-with-speed)
        * [getCurSpeed](#float-getcurspeedvoid)
        * [getMaxSpeed](#uint16_t-getmaxspeedvoid)
        * [setMaxSpeed](#void-setmaxspeeduint16_t-maxspeed)
    * [Working with Acceleration](#working-with-acceleration)
        * [calcMaxAccelDist](#uint32_t-calcmaxacceldistvoid)
        * [decelerate](#void-deceleratevoid)
        * [getAccel](#uint16_t-getaccelvoid)
        * [getAccelDist](#uint32_t-getacceldistvoid)
        * [getDecelDist](#uint32_t-getdeceldistvoid)
        * [getRunDist](#uint32_t-getrundistvoid)
        * [setAccel](#void-setacceluint16_t-accel)
    * [Determining Library/Motor Status](#determining-librarymotor-status)
        * [getDistRemaining](#uint32_t-getdistremainingvoid)
        * [getState](#kissstate_t-getstatevoid)
        * [getTarget](#int32_t-gettargetvoid)
        * [isEnabled](#bool-isenabledvoid)
        * [isMovingForwards](#bool-ismovingforwardsvoid)
    * [Setting/Getting Position Limits](#settinggetting-position-limits)
        * [getForwardLimit](#int32_t-getforwardlimitvoid)
        * [getReverseLimit](#int32_t-getreverselimitvoid)
        * [setForwardLimit](#void-setforwardlimitint32_t-forwardlimit)
        * [setReverseLimit](#void-setreverselimitint32_t-reverselimit)
    * [Other Methods](#other-methods)
        * [disable](#void-disablevoid)
        * [enable](#void-enablevoid)
        * [setPos](#void-setposint32_t-pos)

----

## Installation

### Automatic Installation

1. Download the source code ZIP for the latest release of the library: https://github.com/risitt/kissStepper/releases
2. In the Arduino IDE main menu, select Sketch > Include Library > Add .ZIP Library, then select the downloaded file and click on "Open"

### Manual Installation

1. Download the source code ZIP for the latest release of the library: https://github.com/risitt/kissStepper/releases
2. In the Arduino IDE’s libraries folder (in Windows, this is located in %USERPROFILE%\Documents\Arduino\libraries) extract the contents of the ZIP file into a folder named “kissStepper”
3. Restart the Arduino IDE

----

## Implementation Notes

### Instantiation and Initialization

To use the kissStepper library, you must first include the kissStepper header file into your sketch.

Then, you must create an instance of the kissStepper class before you can interact with your motor. You can create multiple instances to drive multiple motors. This is typically done in the global scope (near the top of your sketch, outside of the loop or setup routines).

This is also where you specify the pinout for the STEP, DIR, and (if desired) ENABLE pins on your motor controller. If you want to control the ENABLE pin yourself, omit the PIN_ENABLE parameter from the kissStepper constructor.

#### Example:
```C++
include <kissStepper.h>
static const uint8_t PIN_DIR = 3;
static const uint8_t PIN_STEP = 4;
static const uint8_t PIN_ENABLE = 7;
kissStepper motor(PIN_DIR, PIN_STEP, PIN_ENABLE); // including the enable pin
// kissStepper motor(PIN_DIR, PIN_STEP); // omitting the enable pin
```

In addition to instantiation, you must initialize the kissStepper library. This is typically done, once, in the setup() routine of your sketch. As with several other Arduino libraries, this is done with a [*begin()*](#void-beginvoid) method. Failing to do this will leave your motor controller in an unknown state for an extended duration, which may cause unwanted behavior.

#### Example:
```C++
void setup(void)
{
    ...
    motor.begin();
    ...
}
```

Once you have included the library header, instantiated the class, and initialized it, you are ready to use the library to control your motor.

### Full-stepping/Microstepping

The library does not handle your motor driver board’s microstep select pins and is completely unaware of how you have set them up. This is a deliberate decision to keep the library simple while ensuring maximum compatibility with a wide variety of motor drivers. This means you need to look at your motor driver board’s specifications/data sheet and set the microstep select pins yourself, either in code or by using pull-up/pull-down resistors.

### Position, Speed, and Acceleration Units of Measurement

When working with the library, you will be using numbers to represent motor position, speed, and acceleration. Because the library has no awareness of whether you are full-stepping or microstepping your motor, or how many full steps represent one revolution of the motor, you may wish to do some simple math to convert to and from more useful units of measurement.

If you are full-stepping your motor, the position used by the library corresponds to full steps, the speed corresponds to full steps/s, and the acceleration corresponds to full step/s<sup>2</sup>. If microstepping, they correspond to microsteps, microstep/s, and microsteps/s<sup>2</sup>, respectively.

If you do not know how many full steps are in one revolution of your motor, look this information up in your motor’s specifications. This number is useful for converting the library’s position index to real-world measurements. For example, if your motor has 200 full steps per revolution, and you are 1/8th microstepping, moving the motor from position 0 to position 3200 would turn it over two revolutions.

### When "Forwards" is Not Forwards

The concepts of “forwards” and “backwards” used by the library may end up being the opposite of what you expect, or even disagree between two motors in the same project.

This depends on how you've wired up your motors and how you've assembled your project. For example, if you are building a robot with two stepper motors controlling opposite wheels, the same target position will drive the wheels in opposite directions.

Currently, you need to compensate for this in your own hardware or software, but I am considering adding a boolean to the kissStepper constructor to reverse the direction logic on a per-instance basis.

### Disabling Acceleration

The main purpose of acceleration is to prevent the motor from stalling at higher speeds, and to ease heavier loads into starting and stopping. If you only run your motor at lower speeds and find that your motor behaves properly without acceleration, you can disable acceleration if you wish.

Setting acceleration to 0 will disable it, but the code which handles acceleration will remain in your program, occupying program and memory space.

To solve this problem, the library includes a version of kissStepper which does not implement acceleration. To use it, simply instantiate the kissStepperNoAccel class instead of the kissStepper class.

### Driving Multiple Motors

There are two methods for driving multiple motors. The first is to use a single microcontroller, set up multiple kissStepper instances, and call multiple [*move()*](#kissstate_t-movevoid) methods within a main loop. A simple example is included in the examples folder (see the TwoMotor sketch). This is the method I recommend for most applications. A 32-bit microcontroller with hardware floating point support will be able to drive multiple motors with ease. For such applications, I can recommend the Teensy platform, as my tests (on a Teensy 3.1) indicate that performance is superb.

Another option is to use separate microcontrollers for operating each motor driver, all controlled by a single master microcontroller. Using SPI, for example, will allow a master microcontroller to send commands to multiple slave microcontrollers, each which use a single instance of the kissStepper library to operate an attached motor driver. Depending on how you implement the communications protocol between the master and slave microcontrollers, this approach can be higher performance than using a single microcontroller, at the expense of additional hardware and complexity. I have not yet attempted this approach and can’t advise you further, but it may be worth trying.

----

## Library Reference
### Instantiation and Initialization
#### kissStepper(uint8_t PIN_DIR, uint8_t PIN_STEP, uint8_t PIN_ENABLE)

The kissStepper class is acceleration-enabled. The constructor takes up to three parameters, specifying which of the microcontroller's pins are connected to the motor controller's DIR, STEP, and ENABLE pins. The PIN_ENABLE parameter is optional, but omitting it is not recommended unless you have permanently enabled the motor controller. If you wish to manually enable and disable the motor controller, you can do so using the [*enable()*](#void-enablevoid) and [*disable()*](#void-disablevoid) methods.

##### Example:
```C++
static const uint8_t PIN_DIR = 3;
static const uint8_t PIN_STEP = 4;
static const uint8_t PIN_ENABLE = 7;
kissStepper motor(PIN_DIR, PIN_STEP, PIN_ENABLE);
```

#### kissStepperNoAccel(uint8_t PIN_DIR, uint8_t PIN_STEP, uint8_t PIN_ENABLE)

The kissStepperNoAccel class is similar to the kissStepper class, but lacks all acceleration/deceleration logic. If you do not need acceleration in your project, using the kissStepperNoAccel class will save program and memory space.

##### Example:
```C++
static const uint8_t PIN_DIR = 3;
static const uint8_t PIN_STEP = 4;
static const uint8_t PIN_ENABLE = 7;
kissStepperNoAccel motor(PIN_DIR, PIN_STEP, PIN_ENABLE);
```

#### void begin(void)

This method initializes an instance of the kissStepper library, and should be called once per instance in the setup routine of your program.

##### Example:
```C++
void setup(void)
{
    ...
    motor.begin();
    ...
}
```

### Essential Methods and Types
#### The kissState_t enum Type

Two of the library’s methods ([*move()*](#kissstate_t-movevoid) and [*getState()*](#kissstate_t-getstatevoid)) return an enum of type [*kissState_t*](#the-kissstate_t-enum-type) which is very useful for determining if the motor is stopped, moving, accelerating, etc. The value returned by the [*move()*](#kissstate_t-movevoid) method is particularly useful for terminating loops once the motor is stopped.

The enum has five possible values, each with a clearly named constant:
* STATE_STOPPED: the motor is stopped (not moving) and the library has not received any new movement commands
* STATE_STARTING: the motor is not yet moving, but the kissStepper library has received a new movement command
* STATE_RUN: the motor is moving at constant speed
* STATE_ACCEL: the motor is accelerating
* STATE_DECEL: the motor is decelerating

##### Example:
```C++
void loop(void)
{
    ...
    motor.prepareMove(3200); // tell the motor to move to position 3200
    while (motor.move() != STATE_STOPPED); // loop move() until the motor has finished moving
    ...
}
```

#### int32_t getPos(void)

Gets the current motor position.
##### Example:
```C++
long curPos = motor.getPos();
```

#### kissState_t move(void)

This method does most of the heavy lifting. **You must call it repeatedly and often** when intending to move the motor. A good place to do this is inside the loop() routine. You can also use it in while loops, or call it at regular intervals using a timer interrupt.

It returns an enum of type [*kissState_t*](#the-kissstate_t-enum-type) which tells you whether the motor is stopped, starting, accelerating, running (constant speed), or decelerating. When used in while loops, the return value can be compared to the STATE_STOPPED constant to terminate the loop at the appropriate time.

##### Example:
```C++
void loop(void)
{
    ...
    motor.prepareMove(1600); // move the motor to position 1600
    while (motor.move() != STATE_STOPPED); // run the motor until it reaches the target
    ...
}
```

#### bool prepareMove(int32_t target)

This method tells the library to move the motor to a specific position (the target). Behind the scenes, it calculates the initial interval between STEP pulses, and how long to accelerate and decelerate (if acceleration is non-zero).

The typical usage is to call this method once, then call [*move()*](#kissstate_t-movevoid) in a loop until the motor has stopped (reached the destination).

Note that this method won't do anything if the motor is already busy. You must either wait until it has stopped, or use [*stop()*](#void-stopvoid) to force it to do so.

Returns TRUE if the motor was stopped and the target position differs from the current position. Otherwise, returns FALSE.

##### Example:
```C++
void loop(void)
{
    ...
    motor.prepareMove(1600); // move the motor to position 1600
    ...
}
```

#### void stop(void)

Tells the motor to stop. The motor will stop very suddenly, even if you are using acceleration. At high speeds or with heavy loads, the lack of deceleration may cause momentum to carry the motor forward, throwing off your motor's indexing. See [*decelerate()*](#void-deceleratevoid) for an alternative.

##### Example:
```C++
motor.stop();
```

### Working with Speed

#### float getCurSpeed(void)

Returns the current speed. Due to acceleration and deceleration, the current speed is often different from the maximum speed. If the motor is not moving, the current speed will be 0.

##### Example:
```C++
float curSpeed = motor.getCurSpeed();
```

#### uint16_t getMaxSpeed(void)

Returns the current maximum speed.

##### Example:
```C++
unsigned int maxSpeed = motor.getMaxSpeed();
```

#### void setMaxSpeed(uint16_t maxSpeed)

Changes the maximum speed (the default is 1600). If using acceleration, the maximum speed will only be reached if the motor is moved over a distance large enough to permit full acceleration to and deceleration from the maximum speed. If not using acceleration, the motor will immediately start at the maximum speed.

This can only be done when the motor is stopped.

##### Example:
```C++
motor.setMaxSpeed(800); // move at a maximum of 800 full steps or microsteps per sec
```

### Working with Acceleration

**Note:** the following methods are unavailable in the kissStepperNoAccel class.

#### uint32_t calcMaxAccelDist(void)

This method returns the distance required to accelerate the motor from a speed of 0 to the maximum speed, at the current acceleration value. Because acceleration and deceleration rates are the same, the value returned by this method can also be interpreted as the distance required to decelerate the motor from maximum speed to 0.

##### Example:
```C++
unsigned long maxAccelDist = motor.calcMaxAccelDist();
```

#### void decelerate(void)

Immediately decelerates the motor down to a speed of 0 using the current acceleration value. If the acceleration value is 0, this method is equivalent to [*stop()*](#void-stopvoid).

Using this method will generally mean that your motor stops short of the target set in [*prepareMove()*](#bool-preparemoveint32_t-target). If that is not your intention, do not use the [*decelerate()*](#void-deceleratevoid) method, and simply wait for the motor to decelerate on its own as it approaches the target.

##### Example:
```C++
motor.decelerate();
```

#### uint16_t getAccel(void)

Returns the acceleration rate.

##### Example:
```C++
unsigned int accel = motor.getAccel();
```

#### uint32_t getAccelDist(void)

Returns the acceleration distance for the current movement, as calculated by [*prepareMove()*](#bool-preparemoveint32_t-target).

##### Example:
```C++
unsigned long accelDist = motor.getAccelDist();
```

#### uint32_t getDecelDist(void)

Returns the deceleration distance for the current movement, as calculated by [*prepareMove()*](#bool-preparemoveint32_t-target).

##### Example:
```C++
unsigned long decelDist = motor.getDecelDist();
```

#### uint32_t getRunDist(void)

Returns the run (constant speed) distance for the current movement, as calculated by [*prepareMove()*](#bool-preparemoveint32_t-target).

##### Example:
```C++
unsigned long runDist = motor.getRunDist();
```

#### void setAccel(uint16_t accel)

Sets a new acceleration rate (the default is 1600). This can only be done when the motor is stopped.

If set to 0, acceleration/deceleration will be disabled. If your program permanently sets acceleration to 0, consider using the kissStepperNoAccel class.

##### Example:
```C++
motor.setAccel(800); // accelerate at 800 full steps or microsteps per sec^2
```

### Determining Library/Motor Status

#### uint32_t getDistRemaining(void)

Returns the absolute difference between the current position and the target position specified in [*prepareMove()*](#bool-preparemoveint32_t-target).

##### Example:
```C++
unsigned long distRemaining = motor.getDistRemaining();
```

#### kissState_t getState(void)

Returns an enum of type [*kissState_t*](#the-kissstate_t-enum-type) which specifies the current state of the library/motor.

##### Example:
```C++
kissState_t curState = motor.getState();
if (curState == STATE_STOPPED)
{
    // motor is stopped, so do something here...
}
```

#### int32_t getTarget(void)

Returns the target position last supplied to [*prepareMove()*](#bool-preparemoveint32_t-target).

##### Example:
```C++
long targetPos = motor.getTarget();
```

#### bool isEnabled(void)

Returns TRUE if the motor controller is enabled, otherwise FALSE. Only meaningful if a PIN_ENABLE parameter was supplied to the kissStepper constructor.

##### Example:
```C++
bool enabled = motor.isEnabled();
```

#### bool isMovingForwards(void)

Returns TRUE if the current or previous movement was "forwards" (positive change in position index). Otherwise returns FALSE.

##### Example:
```C++
bool forwards = motor.isMovingForwards();
```

### Setting/Getting Position Limits

The forward and reverse limits are 32-bit integers which specify the maximum forward and reverse position indexes beyond which the motor will not be allowed to move. You can get or set their value using the methods described below.

Position limits are useful for preventing a motor from moving beyond a range of physical points. They can be used in combination with limit switches to create a calibration routine for your device. All position index parameters specified in calls to [*prepareMove()*](#bool-preparemoveint32_t-target) or [*setPos()*](#void-setposint32_t-pos) are constrained between the forward and reverse limits.

Changes made using either of the setter methods will not retroactively constrain motor movements already underway. If the motor's current position ends up outside of a newly specified set of limits, moving the motor will still be permitted as long as the target position is within the limits. The [*setPos()*](#void-setposint32_t-pos) method may be useful for adjusting the motor's current position index to an acceptable value without moving the motor.

#### int32_t getForwardLimit(void)

Returns the current forward limit.

##### Example:
```C++
long forwardLimit = motor.getForwardLimit();
```

#### int32_t getReverseLimit(void)

Returns the current reverse limit.

##### Example:
```C++
long reverseLimit = motor.getReverseLimit();
```

#### void setForwardLimit(int32_t forwardLimit)

Sets a new forward limit, which will be enforced for all subsequent calls to [*prepareMove()*](#bool-preparemoveint32_t-target) or [*setPos()*](#void-setposint32_t-pos).

##### Example:
```C++
if (forwardLimitSwitchClosed)
{
    motor.stop();
    motor.setForwardLimit(motor.getPos());
}
```

#### void setReverseLimit(int32_t reverseLimit)

Sets a new reverse limit, which will be enforced for all subsequent calls to [*prepareMove()*](#bool-preparemoveint32_t-target) or [*setPos()*](#void-setposint32_t-pos).

##### Example:
```C++
if (reverseLimitSwitchClosed)
{
    motor.stop();
    motor.setReverseLimit(motor.getPos());
}
```

### Other Methods
#### void disable(void)

Stops the motor and disables the motor controller.

##### Example:
```C++
motor.disable();
```

#### void enable(void)

Enables the motor controller. If it is already enabled, nothing untoward happens.

##### Example:
```C++
motor.enable();
```

#### void setPos(int32_t pos)

Changes the current position index without moving the motor. The new position index will be constrated between the forward and reverse position limits. This method may be useful for calibration routines.

##### Example:
```C++
int32_t midpoint = (forwardLimitSwitchIndex + reverseLimitSwitchIndex) / 2;

// send motor to midpoint between limit switches
motor.prepareMove(midpoint);
while (motor.move() != STATE_STOPPED);

// zero current position and apply new limits
motor.setPos(0);
motor.setForwardLimit(forwardLimitSwitchIndex - midpoint);
motor.setReverseLimit(reverseLimitSwitchIndex - midpoint);
```
