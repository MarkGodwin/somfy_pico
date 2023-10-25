// Copyright (c) 2023 Mark Godwin.
// SPDX-License-Identifier: MIT

#pragma once

#include "scheduler.h"

class StatusLed
{
public:
    StatusLed(int pin);

    /// @brief Turn on the LED to full bright
    void TurnOn();

    /// @brief Turn off the LED, and stop pulsing
    void TurnOff();

    /// @brief Set the brightness of the LED
    /// @param level 4096 is max bright, but shouldn't be used for too long
    /// @remarks If pulsing, pulsing will continue so the LED will just flash
    void SetLevel(uint16_t level);

    /// @brief Start pulsing the LED
    void Pulse(int minPulse, int maxPulse, int pulseSpeed);

private:

    void SwitchMode(uint mode);
    uint32_t DoPulse();

    ScheduledTimer _pulseTimer;

    uint _pin;
    uint _stateMachine;
    uint _mode;
    int _currentPulse;
    int _minPulse;
    int _maxPulse;
    int _pulseSpeed;
    absolute_time_t _lastTick;
};
