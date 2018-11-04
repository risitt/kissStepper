# kissStepper API documentation for Arduino and Teensyduino
For a fully working example that implements most of the API methods below, please see the "SerialControl" sketch in the examples folder.

The simplest way to use kissStepper is as follows:

1. Include kissStepper.h into your sketch
2. Create a new instance in the global scope
3. Call [*begin()*](#begin) in your sketch's setup() routine
4. Place [*move()*](#move) in your loop() routine where it will be repeatedly called at regular intervals
5. You can then use [*setMaxSpeed()*](#setMaxSpeed), [*setAccel()*](#setAccel), [*prepareMove()*](#prepareMove), [*stop()*](#stop), [*decelerate()*](#decelerate), and [*getPos()*](#getPos) to set motor speed, move the motor where you want, stop it where you want, and save its current position for later use (such as returning to certain positions)

## Table of Contents
* [Installation](#installation)
    * [Automatic Installation](#installauto)
    * [Manual Installation](#installmanual)
* [Implementation Notes](#Implementation)
    * [Instantiation and Initialization](#setup)
    * [Full-stepping/Microstepping](#stepconfig)
    * [Position, Speed, and Acceleration Units of Measurement](#units)
    * [When "Forwards" is Not Forwards](#direction)
    * [Disabling Acceleration](#noAccel)
    * [Driving Multiple Motors](#multipleMotors)
* [Library Reference](#ref)
    * [Instantiation and Initialization](#refSetup)
        * [The kissStepper Class](#kissStepper)
        * [The kissStepperNoAccel Class](#kissStepperNoAccel)
        * [begin](#begin)
    * [Essential Methods and Types](#essential)
        * [The kissState_t enum Type](#state)
        * [getPos](#getPos)
        * [move](#move)
        * [prepareMove](#prepareMove)
        * [stop](#stop)
    * [Working with Speed](#speed)
        * [getCurSpeed](#getCurSpeed)
        * [getMaxSpeed](#getMaxSpeed)
        * [setMaxSpeed](#setMaxSpeed)
    * [Working with Acceleration](#accel)
        * [calcDecelDist](#calcDecelDist)
        * [calcMaxAccelDist](#calcMaxAccelDist)
        * [decelerate](#decelerate)
        * [getAccel](#getAccel)
        * [getAccelDist](#getAccelDist)
        * [getDecelDist](#getDecelDist)
        * [getRunDist](#getRunDist)
        * [setAccel](#setAccel)
    * [Determining Library/Motor Status](#status)
        * [getDistRemaining](#getDistRemaining)
        * [getState](#getState)
        * [getTarget](#getTarget)
        * [isEnabled](#isEnabled)
        * [isMovingForwards](#isMovingForwards)
    * [Setting/Getting Position Limits](#limits)
        * [getForwardLimit](#getForwardLimit)
        * [getReverseLimit](#getReverseLimit)
        * [setForwardLimit](#setForwardLimit)
        * [setReverseLimit](#setReverseLimit)
    * [Other Methods](#other)
        * [disable](#disable)
        * [enable](#enable)
        * [setPos](#setPos)

----

## <a name="installation">Installation</a>

### <a name="installauto">Automatic Installation</a>

1. Download the source code ZIP for the latest release of the library: https://github.com/risitt/kissStepper/releases
2. In the Arduino IDE main menu, select Sketch > Include Library > Add .ZIP Library, then select the downloaded file and click on "Open"

### <a name="installmanual">Manual Installation</a>

1. Download the source code ZIP for the latest release of the library: https://github.com/risitt/kissStepper/releases
2. In the Arduino IDE’s libraries folder (in Windows, this is located in %USERPROFILE%\Documents\Arduino\libraries) extract the contents of the ZIP file into a folder named “kissStepper”
3. Restart the Arduino IDE

----

## <a name="Implementation">Implementation Notes</a>

### <a name="setup">Instantiation and Initialization</a>

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

In addition to instantiation, you must initialize the kissStepper library. This is typically done, once, in the setup() routine of your sketch. As with several other Arduino libraries, this is done with a [*begin()*](#begin) method. Failing to do this will leave your motor controller in an unknown state for an extended duration, which may cause unwanted behavior.

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

### <a name="stepconfig">Full-stepping/Microstepping</a>

The library does not handle your motor driver board’s microstep select pins and is completely unaware of how you have set them up. This is a deliberate decision to keep the library simple while ensuring maximum compatibility with a wide variety of motor drivers. This means you need to look at your motor driver board’s specifications/data sheet and set the microstep select pins yourself, either in code or by using pull-up/pull-down resistors.

### <a name="units">Position, Speed, and Acceleration Units of Measurement</a>

When working with the library, you will be using numbers to represent motor position, speed, and acceleration. Because the library has no awareness of whether you are full-stepping or microstepping your motor, or how many full steps represent one revolution of the motor, you may wish to do some simple math to convert to and from more useful units of measurement.

If you are full-stepping your motor, the position used by the library corresponds to full steps, the speed corresponds to full steps/s, and the acceleration corresponds to full step/s^2^. If microstepping, they correspond to microsteps, microstep/s, and microsteps/s^2^, respectively.

If you do not know how many full steps are in one revolution of your motor, look this information up in your motor’s specifications. This number is useful for converting the library’s position index to real-world measurements. For example, if your motor has 200 full steps per revolution, and you are 1/8th microstepping, moving the motor from position 0 to position 3200 would turn it over two revolutions.

### <a name="direction">When "Forwards" is Not Forwards</a>

The concepts of “forwards” and “backwards” used by the library may end up being the opposite of what you expect, or even disagree between two motors in the same project.

This depends on how you've wired up your motors and how you've assembled your project. For example, if you are building a robot with two stepper motors controlling opposite wheels, the same target position will drive the wheels in opposite directions.

Currently, you need to compensate for this in your own hardware or software, but I am considering adding a boolean to the kissStepper constructor to reverse the direction logic on a per-instance basis.

### <a name="noAccel">Disabling Acceleration</a>

The main purpose of acceleration is to prevent the motor from stalling at higher speeds, and to ease heavier loads into starting and stopping. If you only run your motor at lower speeds and find that your motor behaves properly without acceleration, you can disable acceleration if you wish.

Setting acceleration to 0 will disable it, but the code which handles acceleration will remain in your program, occupying program and memory space.

To solve this problem, the library includes a version of kissStepper which does not implement acceleration. To use it, simply instantiate the kissStepperNoAccel class instead of the kissStepper class.

### <a name="multipleMotors">Driving Multiple Motors</a>

There are two methods for driving multiple motors. The first is to use a single microcontroller, set up multiple kissStepper instances, and call multiple [*move()*](#move) methods within a main loop. A simple example is included in the examples folder (see the TwoMotor sketch). This is the method I recommend for most applications. A 32-bit microcontroller with hardware floating point support will be able to drive multiple motors with ease. For such applications, I can recommend the Teensy platform, as my tests (on a Teensy 3.1) indicate that performance is superb.

Another option is to use separate microcontrollers for operating each motor driver, all controlled by a single master microcontroller. Using SPI, for example, will allow a master microcontroller to send commands to multiple slave microcontrollers, each which use a single instance of the kissStepper library to operate an attached motor driver. Depending on how you implement the communications protocol between the master and slave microcontrollers, this approach can be higher performance than using a single microcontroller, at the expense of additional hardware and complexity. I have not yet attempted this approach and can’t advise you further, but it may be worth trying.

----

## <a name="ref">Library Reference</a>
### <a name="refSetup">Instantiation and Initialization</a>
#### <a name="kissStepper">kissStepper(uint8_t PIN_DIR, uint8_t PIN_STEP, uint8_t PIN_ENABLE)</a>

The kissStepper class is acceleration-enabled. The constructor takes up to three parameters, specifying which of the microcontroller's pins are connected to the motor controller's DIR, STEP, and ENABLE pins. The PIN_ENABLE parameter is optional, but omitting it is not recommended unless you have permanently enabled the motor controller. If you wish to manually enable and disable the motor controller, you can do so using the [*enable()*](#enable) and [*disable()*](#disable) methods.

##### Example:
```C++
static const uint8_t PIN_DIR = 3;
static const uint8_t PIN_STEP = 4;
static const uint8_t PIN_ENABLE = 7;
kissStepper motor(PIN_DIR, PIN_STEP, PIN_ENABLE);
```

#### <a name="kissStepperNoAccel">kissStepperNoAccel(uint8_t PIN_DIR, uint8_t PIN_STEP, uint8_t PIN_ENABLE)</a>

The kissStepperNoAccel class is similar to the kissStepper class, but lacks all acceleration/deceleration logic. If you do not need acceleration in your project, using the kissStepperNoAccel class will save program and memory space.

##### Example:
```C++
static const uint8_t PIN_DIR = 3;
static const uint8_t PIN_STEP = 4;
static const uint8_t PIN_ENABLE = 7;
kissStepperNoAccel motor(PIN_DIR, PIN_STEP, PIN_ENABLE);
```

#### <a name="begin">void begin(void)</a>

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

### <a name="essential">Essential Methods and Types</a>
#### <a name="state">The kissState_t enum Type</a>

Two of the library’s methods ([*move()*](#move) and [*getState()*](#getState)) return an enum of type [*kissState_t*](#state) which is very useful for determining if the motor is stopped, moving, accelerating, etc. The value returned by the [*move()*](#move) method is particularly useful for terminating loops once the motor is stopped.

The enum has five possible values, each with a clearly named constant:
* STATE_STOPPED: the motor is stopped (not moving) and the library has not received any new movement commands
* STATE_STARTING: the motor is not moving, but the kissStepper library is ready to start moving it
* STATE_RUN: the motor is moving at constant speed
* STATE_ACCEL: the motor is accelerating
* STATE_DECEL: the motor is decelerating

##### Example:
```C++
void loop(void)
{
    ...
    motor.prepareMove(3200); // tell the motor to move 3200 units forwards
    while (motor.move() != STATE_STOPPED); // loop move() until the motor has finished moving
    ...
}
```

#### <a name="getPos">int32_t getPos(void)</a>

Gets the current motor position.
##### Example:
```C++
long curPos = motor.getPos();
```

#### <a name="move">kissState_t move(void)</a>

This method does most of the heavy lifting. **You must call it repeatedly and often** when intending to move the motor. A good place to do this is inside the loop() routine. You can also use it in while loops, or call it at regular intervals using a timer interrupt.

It returns an enum of type [*kissState_t*](#state) which tells you whether the motor is stopped, starting, accelerating, running (constant speed), or decelerating. When used in while loops, the return value can be compared to the STATE_STOPPED constant to terminate the loop at the appropriate time.

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

#### <a name="prepareMove">bool prepareMove(int32_t target)</a>

This method tells the library to move the motor to a specific position (the target). Behind the scenes, it calculates the initial interval between STEP pulses, and how long to accelerate and decelerate (if acceleration is non-zero).

The typical usage is to call this method once, then call [*move()*](#move) in a loop until the motor has stopped (reached the destination).

Note that this method won't do anything if the motor is already busy. You must either wait until it has stopped, or use [*stop()*](#stop) to force it to do so.

Returns TRUE if the motor was stopped and the target position differs from the current position. Otherwise, returns FALSE.

##### Example:
```C++
void loop(void)
{
    ...
    motor.prepareMove(1600); // move the motor forwards through 1600 STEP pulses
    ...
}
```

#### <a name="stop">void stop(void)</a>

Tells the motor to stop. The motor will stop very suddenly, even if you are using acceleration. At high speeds or with heavy loads, the lack of deceleration may cause momentum to carry the motor forward, throwing off your motor's indexing. See [*decelerate()*](#decelerate) for an alternative.

##### Example:
```C++
motor.stop();
```

### <a name="speed">Working with Speed</a>

#### <a name="getCurSpeed">float getCurSpeed(void)</a>

Returns the current speed. Due to acceleration and deceleration, the current speed is often different from the maximum speed. If the motor is not moving, the current speed will be 0.

##### Example:
```C++
float curSpeed = motor.getCurSpeed();
```

#### <a name="getMaxSpeed">uint16_t getMaxSpeed(void)</a>

Returns the current maximum speed.

##### Example:
```C++
unsigned int maxSpeed = motor.getMaxSpeed();
```

#### <a name="setMaxSpeed">void setMaxSpeed(uint16_t maxSpeed)</a>

Changes the maximum speed (the default is 1600). If using acceleration, the maximum speed will only be reached if the motor is moved over a distance large enough to permit full acceleration to and deceleration from the maximum speed. If not using acceleration, the motor will immediately start at the maximum speed.

This can only be done when the motor is stopped.

##### Example:
```C++
motor.setMaxSpeed(800); // move at a maximum of 800 full steps or microsteps per sec
```

### <a name="accel">Working with Acceleration</a>

**Note:** the following methods are unavailable in the kissStepperNoAccel class.

#### <a name="calcDecelDist">uint32_t calcDecelDist(void)</a>

This method returns the distance required to decelerate the motor from its current speed to 0, at the current acceleration value.

##### Example:
```C++
unsigned long decelDist = motor.calcDecelDist();
```

#### <a name="calcMaxAccelDist">uint32_t calcMaxAccelDist(void)</a>

This method returns the distance required to accelerate the motor from a speed of 0 to the maximum speed, at the current acceleration value. Because acceleration and deceleration rates are the same, the value returned by this method can also be interpreted as the distance required to decelerate the motor from maximum speed to 0.

##### Example:
```C++
unsigned long maxAccelDist = motor.calcMaxAccelDist();
```

#### <a name="decelerate">void decelerate(void)</a>

Immediately decelerates the motor down to a speed of 0 using the current acceleration value. If the acceleration value is 0, this method is equivalent to [*stop()*](#stop).

Using this method will generally mean that your motor stops short of the target set in [*prepareMove()*](#prepareMove). If that is not your intention, do not use the [*decelerate()*](#decelerate) method, and simply wait for the motor to decelerate on its own as it approaches the target.

##### Example:
```C++
motor.decelerate();
```

#### <a name="getAccel">uint16_t getAccel(void)</a>

Returns the acceleration rate.

##### Example:
```C++
unsigned int accel = motor.getAccel();
```

#### <a name="getAccelDist">uint32_t getAccelDist(void)</a>

Returns the acceleration distance for the current movement, as calculated by [*prepareMove()*](#prepareMove).

##### Example:
```C++
unsigned long accelDist = motor.getAccelDist();
```

#### <a name="getDecelDist">uint32_t getDecelDist(void)</a>

Returns the deceleration distance for the current movement, as calculated by [*prepareMove()*](#prepareMove).

##### Example:
```C++
unsigned long decelDist = motor.getDecelDist();
```

#### <a name="getRunDist">uint32_t getRunDist(void)</a>

Returns the run (constant speed) distance for the current movement, as calculated by [*prepareMove()*](#prepareMove).

##### Example:
```C++
unsigned long runDist = motor.getRunDist();
```

#### <a name="setAccel">void setAccel(uint16_t accel)</a>

Sets a new acceleration rate (the default is 1600). This can only be done when the motor is stopped.

If set to 0, acceleration/deceleration will be disabled. If your program permanently sets acceleration to 0, consider using the kissStepperNoAccel class.

##### Example:
```C++
motor.setAccel(800); // accelerate at 800 full steps or microsteps per sec^2
```

### <a name="status">Determining Library/Motor Status</a>

#### <a name="getDistRemaining">uint32_t getDistRemaining(void)</a>

Returns the absolute difference between the current position and the target position specified in [*prepareMove()*](#prepareMove).

##### Example:
```C++
unsigned long distRemaining = motor.getDistRemaining();
```

#### <a name="getState">kissState_t getState(void)</a>

Returns an enum of type [*kissState_t*](#state) which specifies the current state of the library/motor. Possible values and meaning are:

* STATE_STOPPED: the motor is stopped (not moving) and the library has not received any new movement commands
* STATE_STARTING: the motor is not moving, but the kissStepper library is ready to start moving it
* STATE_RUN: the motor is moving at constant speed
* STATE_ACCEL: the motor is accelerating
* STATE_DECEL: the motor is decelerating

##### Example:
```C++
kissState_t curState = motor.getState();
if (curState == STATE_STOPPED)
{
    // motor is stopped, so do something here...
}
```

#### <a name="getTarget">int32_t getTarget(void)</a>

Returns the target position last supplied to [*prepareMove()*](#prepareMove).

##### Example:
```C++
long targetPos = motor.getTarget();
```

#### <a name="isEnabled">bool isEnabled(void)</a>

Returns TRUE if the motor controller is enabled, otherwise FALSE. Only meaningful if a PIN_ENABLE parameter was supplied to the kissStepper constructor.

##### Example:
```C++
bool enabled = motor.isEnabled();
```

#### <a name="isMovingForwards">bool isMovingForwards(void)</a>

Returns TRUE if the current or previous movement was "forwards" (positive change in position index). Otherwise returns FALSE.

##### Example:
```C++
bool forwards = motor.isMovingForwards();
```

### <a name="limits">Setting/Getting Position Limits</a>

The forward and reverse limits are 32-bit integers which specify the maximum forward and reverse position indexes beyond which the motor will not be allowed to move. You can get or set their value using the methods described below.

Position limits are useful for preventing a motor from moving beyond a range of physical points. They can be used in combination with limit switches to create a calibration routine for your device. All position index parameters specified in calls to [*prepareMove()*](#prepareMove) or [*setPos()*](#setPos) are constrained between the forward and reverse limits.

Changes made using either of the setter methods will not retroactively constrain motor movements already underway. If the motor's current position ends up outside of a newly specified set of limits, moving the motor will still be permitted as long as the target position is within the limits. The [*setPos()*](#setPos) method may be useful for adjusting the motor's current position index to an acceptable value without moving the motor.

#### <a name="getForwardLimit">int32_t getForwardLimit(void)</a>

Returns the current forward limit.

##### Example:
```C++
long forwardLimit = motor.getForwardLimit();
```

#### <a name="getReverseLimit">int32_t getReverseLimit(void)</a>

Returns the current reverse limit.

##### Example:
```C++
long reverseLimit = motor.getReverseLimit();
```

#### <a name="setForwardLimit">void setForwardLimit(int32_t forwardLimit)</a>

Sets a new forward limit, which will be enforced for all subsequent calls to [*prepareMove()*](#prepareMove) or [*setPos()*](#setPos).

##### Example:
```C++
if (forwardLimitSwitchClosed)
{
    motor.stop();
    motor.setForwardLimit(motor.getPos());
}
```

#### <a name="setReverseLimit">void setReverseLimit(int32_t reverseLimit)</a>

Sets a new reverse limit, which will be enforced for all subsequent calls to [*prepareMove()*](#prepareMove) or [*setPos()*](#setPos).

##### Example:
```C++
if (reverseLimitSwitchClosed)
{
    motor.stop();
    motor.setReverseLimit(motor.getPos());
}
```

### <a name="other">Other Methods</a>
#### <a name="disable">void disable(void)</a>

Stops the motor and disables the motor controller.

##### Example:
```C++
motor.disable();
```

#### <a name="enable">void enable(void)</a>

Enables the motor controller. If it is already enabled, nothing untoward happens.

##### Example:
```C++
motor.enable();
```

#### <a name="setPos">void setPos(int32_t pos)</a>

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
