# Keep it Simple Stepper (kissStepper)
This is a lightweight Arduino library for stepper motor controllers that use STEP/DIR interfaces, such as the Allegro and TI chips, the Easy Driver, Big Easy Driver, and Pololu DRV8825

Written by Rylee Isitt

## Features:
* Fast performance, uses port access to toggle the STEP pin
* Speed is set in full steps per second
* Linear acceleration/deceleration set in steps/sec/sec
* Accurate speed and acceleration timing
* Support for drivers with up to 1/128 microstepping and up to 3 microstep/drive mode select pins
* Fully reconfigurable microstep/drive mode select pin states
	* The default configuration is for the Easy Driver, Big Easy Driver, and several of the Allegro chips
* Consistent speed and position index regardless of drive mode
* Can operate with as few as two pins (STEP and DIR), but can also control the ENABLE pin and the microstep/drive mode select pins

## License:
GNU Lesser General Public License (LGPL) V2.1

**Please see the LICENSE file for details**

## Installation Instructions:
1. Download the latest release
2. Extract the contents to your desktop
3. Rename the containing folder "kissStepper"
4. Move the folder to your Arduino libraries folder
	* On a Windows PC, this is likely to be C:\Program Files (x86)\Arduino\libraries
5. Restart the Arduino IDE if it was running
6. Include the library in your sketch: **#include <kissStepper.h>**
7. Read the API Documentation, or see the example sketches for information on how to use the library