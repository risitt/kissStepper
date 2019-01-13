/*
kissStepper - a lightweight library for the Easy Driver, Big Easy Driver, Allegro stepper motor drivers and others that use a Step/Dir interface
Written by Rylee Isitt. September 21, 2015
License: GNU Lesser General Public License (LGPL) V2.1

Despite the existence of several excellent libraries for driving stepper motors, I created this one to fulfill the following needs:
- Simplicity
- Handling of enable, step, and dir pins
- Based around an external loop
- Approximately linear acceleration using a fast algorithm
- High step frequency (or reasonably so, given the overhead involved)
- Use AVR/ARM libraries and port access to increase performance while keeping the API Arduino-friendly
- Teensy (Teensyduino) compatibility

Acceleration approximation math is based on Aryeh Eiderman's "Real Time Stepper Motor Linear Ramping Just by Addition and Multiplication", available at http://hwml.com/LeibRamp.pdf
*/

#ifndef kissStepper_H
#define kissStepper_H

#include <Arduino.h>

// the order of enums allows some simple tests:
// if > STATE_STARTING, motor is in motion
// if > STATE_RUN, motor is accelerating or decelerating
enum kissState_t: uint8_t
{
    STATE_STOPPED = 0,
    STATE_STARTING = 1,
    STATE_RUN = 2,
    STATE_ACCEL = 3,
    STATE_DECEL = 4
};

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------
// kissStepper without acceleration
// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

class kissStepperNoAccel
{
public:
    kissStepperNoAccel(uint8_t PIN_DIR, uint8_t PIN_STEP, uint8_t PIN_ENABLE = 255);
    ~kissStepperNoAccel(void) {};

    bool prepareMove(int32_t target);
    kissState_t move(void);
    void stop(void);

    uint16_t getCurSpeed(void)
    {
        if (m_kissState == STATE_RUN)
            return m_maxSpeed;
        else
            return 0;
    }
    kissState_t getState(void)
    {
        return m_kissState;
    }
    int32_t getPos(void)
    {
        if (m_forwards)
            return m_pos + m_distMoved;
        else
            return m_pos - m_distMoved;
    }
    bool isEnabled(void)
    {
        return m_enabled;
    }
    bool isMovingForwards(void)
    {
        return m_forwards;
    }
    void begin(void);
    void enable(void);
    void disable(void);

    void setPos(int32_t pos)
    {
        if (m_kissState == STATE_STOPPED)
            m_pos = constrain(pos, m_reverseLimit, m_forwardLimit);
    }
    int32_t getTarget(void)
    {
        if (m_kissState == STATE_STOPPED)
            return m_pos;
        else if (m_forwards)
            return m_pos + m_distTotal;
        else
            return m_pos - m_distTotal;
    }
    uint32_t getDistRemaining(void)
    {
        return m_distTotal - m_distMoved;
    }
    void setForwardLimit(int32_t forwardLimit)
    {
        m_forwardLimit = forwardLimit;
    }
    void setReverseLimit(int32_t reverseLimit)
    {
        m_reverseLimit = reverseLimit;
    }
    int32_t getForwardLimit(void)
    {
        return m_forwardLimit;
    }
    int32_t getReverseLimit(void)
    {
        return m_reverseLimit;
    }
    void setMaxSpeed(uint16_t maxSpeed)
    {
        if (m_kissState == STATE_STOPPED) m_maxSpeed = maxSpeed;
    }
    uint16_t getMaxSpeed(void)
    {
        return m_maxSpeed;
    }

protected:
    void setDir(bool forwards)
    {
        m_forwards = forwards;
        digitalWrite(PIN_DIR, (forwards ? LOW : HIGH));
    }
    void updatePos(void)
    {
        if (m_forwards)
            m_pos += m_distMoved;
        else
            m_pos -= m_distMoved;
        m_distMoved = 0;
    }
    static const uint32_t ONE_SECOND = 1000000UL;

    // F_CPU / ONE_SECOND will give number of cycles in 1 us
    // Multiply by PULSE_WIDTH_US to get number of cycles for a pulse
    // Subtract 6 to account for the number of cycles between setting port high and low (between st instructions)
    // Divide by 3 to get number of loops of _delay_loop_1 needed
    static const uint8_t PULSE_WIDTH_US = 2; // desired width of step pulse (high) in us
    static const uint8_t PULSE_WIDTH_CYCLES = (F_CPU / ONE_SECOND) * PULSE_WIDTH_US;
    static const uint8_t STEP_RESET_CYCLES = 6; // number of cycles in between step high and low minus busy wait (measured with logic analyser)
    static const uint8_t PULSE_WIDTH_LOOP_COUNT = ((PULSE_WIDTH_CYCLES - STEP_RESET_CYCLES) / 3) + 1; // cycles for width of step pulse

    static const int32_t DEFAULT_FORWARD_LIMIT = 2147483647L;
    static const int32_t DEFAULT_REVERSE_LIMIT = -2147483648L;
    static const uint16_t DEFAULT_SPEED = 1600;
    static const uint16_t INTERVAL_CORRECTION_INCREMENT = 255;

    int32_t m_forwardLimit;
    int32_t m_reverseLimit;
    uint16_t m_maxSpeed;

    const uint8_t PIN_DIR;
    const uint8_t PIN_STEP;
    const uint8_t PIN_ENABLE;

    kissState_t m_kissState;
    uint32_t m_distTotal, m_distMoved;
    bool m_forwards;
    int32_t m_pos;

    const uint8_t m_stepBit;
    uint8_t volatile * const m_stepOut;
    uint32_t m_stepIntervalWhole;
    uint16_t m_stepIntervalRemainder;
    uint16_t m_stepIntervalCorrectionCounter;
    bool m_enabled;
    uint32_t m_lastStepTime;
    bool m_init;
};

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------
// kissStepper WITH acceleration
// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------

class kissStepper: public kissStepperNoAccel
{
public:
    kissStepper(uint8_t PIN_DIR, uint8_t PIN_STEP, uint8_t PIN_ENABLE = 255);
    ~kissStepper(void) {};
    bool prepareMove(int32_t target);
    kissState_t move(void);
    void stop(void);
    
    uint16_t getCurSpeed(void)
    {
        if (m_kissState == STATE_RUN)
            return m_maxSpeed;
        else if (m_kissState > STATE_STARTING)
        {
            uint32_t curSpeed = ONE_SECOND / m_stepIntervalWhole;
            if (curSpeed > m_maxSpeed) curSpeed = m_maxSpeed;
            return curSpeed;
        }
        else
            return 0;
    }
    
    void decelerate(void);
    uint32_t calcMaxAccelDist(void)
    {
        if (m_accel > 0)
            return ((uint32_t)m_maxSpeed * m_maxSpeed) / (2UL * m_accel);
        else
            return 0;
    }
    uint32_t getAccelDist(void)
    {
        return m_distAccel;
    }
    uint32_t getRunDist(void)
    {
        return m_distRun - m_distAccel;
    }
    uint32_t getDecelDist(void)
    {
        return m_distTotal - m_distRun;
    }
    void setAccel(uint16_t accel)
    {
        if (m_kissState == STATE_STOPPED) m_accel = accel;
    }
    uint16_t getAccel(void)
    {
        return m_accel;
    }
    uint16_t getTopSpeed(void);

protected:

    static const uint16_t DEFAULT_ACCEL = 1600;
    uint32_t m_distAccel, m_distRun;
    uint32_t m_topSpeedStepInterval;
    float m_stepInterval;
    float m_constMult;
    uint16_t m_accel;

private:

    /*
       ----------------------------------------------------------------------------------------------------

           To strike a balance between accuracy and performance, this library uses a set of approximations
           for calculating stepInterval when accelerating/decelerating. Although this does use floating point
           math, it is a drastic improvement over exact calculations and better than anything else I've tried.

           There is probably room for further improvement (fixed point or integer math?) but this is good enough.

           exact:
               stepInterval = ONE_SECOND / newSpeed
               curSpeed = ONE_SECOND / stepInterval
               newSpeed = sqrt(curSpeed^2 + 2a)
               stepInterval = ONE_SECOND / sqrt(curSpeed^2 + 2a)

           approximations:
               constMult = accel / (ONE_SECOND * ONE_SECOND)
               q = constMult*stepInterval*stepInterval
               set q to negative if accelerating

               good precision, fast: stepInterval *= 1.0 + q
               better precision, slower: stepInterval *= 1.0 + q + q*q
               best precision, slowest: stepInterval *= 1.0 + q + 1.5*q*q

       ----------------------------------------------------------------------------------------------------
       */

    float accelStep(float stepInterval, float constMult)
    {
        float q = -constMult*stepInterval*stepInterval;
        return stepInterval * (1.0 + q);
        // return stepInterval * (1.0 + q + q*q); // better accuracy
        // return stepInterval * (1.0 + q + 1.5*q*q); // best accuracy
    }

    float decelStep(float stepInterval, float constMult)
    {
        float q = constMult*stepInterval*stepInterval;
        return stepInterval * (1.0 + q);
        // return stepInterval * (1.0 + q + q*q); // better accuracy
        // return stepInterval * (1.0 + q + 1.5*q*q); // best accuracy (doesn't work for decel, investigate why)
    }

};

#endif
