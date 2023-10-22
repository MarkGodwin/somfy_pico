#include "pico/stdlib.h"
#include <stdio.h>
#include "statusLed.h"
#include "hardware/pio.h"
#include "pwm.pio.h"
#include "pulsefade.pio.h"


// From the Pico samples. We can use the same PWM program for all three output pins independently.
/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#define DEFAULT_PIO pio0
static uint nextStateMachine = 0;

uint do_pwm_init() {
    // Lazy
    static uint pwmOffset = -1;
    if(pwmOffset != -1)
        return pwmOffset;

    // Todo: Get free state machine. How?
    pwmOffset = pio_add_program(DEFAULT_PIO, &pwm_program);
    return pwmOffset;
};

uint do_pulsefade_init()
{
    static uint pulseFadeOffset = -1;
    if(pulseFadeOffset != -1)
        return pulseFadeOffset;
    pulseFadeOffset = pio_add_program(DEFAULT_PIO, &pulsefade_program);
    return pulseFadeOffset;
}

StatusLed::StatusLed(int pin)
:   _pin(pin),
    _stateMachine(nextStateMachine++),
    _mode(0)
{
}

void StatusLed::SwitchMode(uint mode)
{
    if(mode == _mode)
        return;
    if(_mode != 0)
    {
        // Disable the current state machine
        pio_sm_set_enabled(DEFAULT_PIO, _stateMachine, false);
        // Turn off the LED
        pio_sm_exec(DEFAULT_PIO, _stateMachine, pio_encode_nop() | pio_encode_sideset_opt(1, 0));

    }

    if(mode == 1)
    {
        auto offset = do_pwm_init();
        pwm_program_init(DEFAULT_PIO, _stateMachine, offset, _pin);

        // Write `period` to the input shift register
        pio_sm_set_enabled(DEFAULT_PIO, _stateMachine, false);
        pio_sm_put_blocking(DEFAULT_PIO, _stateMachine, 4096);
        pio_sm_exec(DEFAULT_PIO, _stateMachine, pio_encode_pull(false, false));
        pio_sm_exec(DEFAULT_PIO, _stateMachine, pio_encode_out(pio_isr, 32));
        pio_sm_set_enabled(DEFAULT_PIO, _stateMachine, true);
    }
    // else if(mode == 2)
    // {
    //     auto offset = do_pulsefade_init();
    //     pulsefade_program_init(DEFAULT_PIO, _stateMachine, offset, _pin);
    //     pio_sm_set_enabled(DEFAULT_PIO, _stateMachine, false);
    //     pio_sm_set_enabled(DEFAULT_PIO, _stateMachine, true);
    // }
    _mode = mode;
}

void StatusLed::TurnOn()
{
    SetLevel(1024);
}

void StatusLed::TurnOff()
{
    SwitchMode(0);
}

void StatusLed::SetLevel(uint16_t level)
{
    if(level == 0)
    {
        SwitchMode(0);
    }
    else
    {
        SwitchMode(1);
        // Write `level` to TX FIFO. State machine will copy this into X.
        pio_sm_put_blocking(DEFAULT_PIO, _stateMachine, (uint)level);   
    }
}

// void StatusLed::Pulse(uint delay)
// {
//     SwitchMode(2);
//     pio_sm_put_blocking(DEFAULT_PIO, _stateMachine, 268435456);

// }
