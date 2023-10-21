#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <algorithm>
#include "pico.h"
#include "pico/stdlib.h"
#include "pico/mem_ops.h"
#include "hardware/spi.h"
#include "radio.h"
#include "remote.h"
#include "deviceConfig.h"
#include "commandQueue.h"
#include "blinds.h"

SomfyRemote::SomfyRemote(
    std::shared_ptr<RadioCommandQueue> commandQueue,
        std::shared_ptr<Blinds> blinds,
        std::string name,
        uint32_t remoteId,
        uint16_t rollingCode,
        std::vector<uint16_t> associatedBlinds)
:   _commandQueue(std::move(commandQueue)),
    _blinds(std::move(blinds)),
    _remoteName(std::move(name)),
    _remoteId(remoteId),
    _rollingCode(rollingCode),
    _associatedBlinds(std::move(associatedBlinds))
{
}

void SomfyRemote::PressButtons(SomfyButton buttons, uint16_t repeat)
{
    _commandQueue->QueueCommand(_remoteId, _rollingCode++, buttons, repeat);

    // Now tell all our connected blinds that we've sent a command
    for(auto blindId : _associatedBlinds)
    {
        _blinds->GetBlind(blindId)->ButtonsPressed(buttons);
    }
}

void SomfyRemote::AssociateBlind(uint16_t blindId)
{
    if(std::find(_associatedBlinds.begin(), _associatedBlinds.end(), blindId) != _associatedBlinds.end())
        // Already present :/
        return;
    _associatedBlinds.push_back(blindId);
}

void SomfyRemote::DisassociateBlind(uint16_t blindId)
{
    auto pos = std::find(_associatedBlinds.begin(), _associatedBlinds.end(), blindId);
    if( pos == _associatedBlinds.end())
        return;

    _associatedBlinds.erase(pos);
}
