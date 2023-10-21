#include "pico/stdlib.h"

#include "remotes.h"
#include "deviceConfig.h"

const uint32_t BaseRemoteId = 0x270000;

SomfyRemotes::SomfyRemotes(std::shared_ptr<DeviceConfig> config, std::shared_ptr<Blinds> blinds, std::shared_ptr<RadioCommandQueue> commandQueue)
:   _config(std::move(config)),
    _blinds(std::move(blinds)),
    _commandQueue(std::move(commandQueue))
{

    uint32_t count;
    const uint16_t *remoteIds = _config->GetRemoteIds(&count);

    printf("There are %d remotes registered\n", count);

    _nextId = BaseRemoteId + 1;
    for(auto a = 0; a < count; a++)
    {
        auto id = remoteIds[a] | BaseRemoteId;
        if(id >= _nextId)
            _nextId = id;
        auto cfg = _config->GetRemoteConfig(id);
        std::vector<uint16_t> assocBlinds(cfg->blinds, cfg->blinds + cfg->blindCount);
        _remotes.insert({cfg->remoteId, std::make_shared<SomfyRemote>(_commandQueue, _blinds, cfg->remoteName, cfg->remoteId, cfg->rollingCode, assocBlinds)});
    }
}

std::shared_ptr<SomfyRemote> SomfyRemotes::GetRemote(uint32_t remoteId)
{
    auto entry = _remotes.find(remoteId);
    if(entry == _remotes.end())
        return nullptr;
    return entry->second;
}

std::shared_ptr<SomfyRemote> SomfyRemotes::CreateRemote(std::string remoteName)
{
    auto id = _nextId++;
    auto newRemote = std::make_shared<SomfyRemote>(_commandQueue, _blinds, remoteName, id, 1, std::vector<uint16_t>());
    _remotes.insert({id, newRemote});

    return newRemote;
}