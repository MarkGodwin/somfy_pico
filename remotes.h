
#pragma once

#include "remote.h"
#include <list>
#include <memory>
#include <map>

class DeviceConfig;
class RadioCommandQueue;
class Blinds;

class SomfyRemotes
{
    public:
        SomfyRemotes(std::shared_ptr<DeviceConfig> config, std::shared_ptr<Blinds> blinds, std::shared_ptr<RadioCommandQueue> commandQueue);

        std::shared_ptr<SomfyRemote> GetRemote(uint32_t remoteId);
        std::shared_ptr<SomfyRemote> CreateRemote(std::string remoteName);

    private:
        std::shared_ptr<DeviceConfig> _config;
        std::shared_ptr<Blinds> _blinds;
        std::shared_ptr<RadioCommandQueue> _commandQueue;
        std::map<uint32_t, std::shared_ptr<SomfyRemote>> _remotes;

        uint32_t _nextId;
};
