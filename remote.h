
#pragma once

#include <memory>
#include <vector>

class RadioCommandQueue;

enum SomfyButton : int
{
    My = 1,
    Up = 2,
    Down = 4,
    Prog = 8,
    SunDetectorOn = My | Prog,
    SunDetectorOff = Up | Prog
};

const int ShortPress = 3;
const int LongPress = 12;

class Blinds;
class RemoteConfig;

class SomfyRemote
{
public:
    SomfyRemote(
        std::shared_ptr<RadioCommandQueue> commandQueue,
        std::shared_ptr<Blinds> blinds,
        std::string name,
        uint32_t remoteId,
        uint16_t rollingCode,
        std::vector<uint16_t> associatedBlinds);

    const std::string &GetName() { return _remoteName; }
    uint16_t GetRollingCode() { return _rollingCode; }
    uint32_t GetRemoteId() { return _remoteId; }
    const std::vector<uint16_t> &GetAssociatedBlinds() { return _associatedBlinds; }

    void SetName(std::string name) { _remoteName = std::move(name); }
    void AssociateBlind(uint16_t blindId);
    void DisassociateBlind(uint16_t blindId);

    // Press buttons on the controller. Note that buttons can be chorded.
    void PressButtons(SomfyButton buttons, uint16_t repeat);

private:
    std::shared_ptr<RadioCommandQueue> _commandQueue;
    std::shared_ptr<Blinds> _blinds;
    std::string _remoteName;
    uint32_t _remoteId;
    uint16_t _rollingCode;
    std::vector<uint16_t> _associatedBlinds;
};
