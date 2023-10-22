#pragma once

class StatusLed
{
public:
    StatusLed(int pin);

    void TurnOn();
    void TurnOff();
    void SetLevel(uint16_t level);
    //void Pulse(uint delay);

private:

    void SwitchMode(uint mode);

    uint _pin;
    uint _stateMachine;
    uint _mode;
};
