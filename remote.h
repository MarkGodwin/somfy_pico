
#pragma once

class RFM69Radio;

enum SomfyButton
{
    My = 1,
    Up = 2,
    Down = 4,
    Prog = 8,
    SunDetectorOn = My | Prog,
    SunDetectorOff = Up | Prog
};


class SomfyRemote
{
public:
    SomfyRemote(RFM69Radio *radio, uint32_t remoteId, uint16_t rollingCode);

    uint16_t GetRollingCode() { return _rollingCode; }

    // Press buttons on the controller. Note that buttons can be chorded.
    void PressButtons(SomfyButton buttons, uint16_t repeat);

private:
    RFM69Radio *_radio;
    uint32_t _remoteId;
    uint16_t _rollingCode;
};
