

#include "pico/stdlib.h"

#include "blinds.h"
#include "deviceConfig.h"
#include "remotes.h"


Blinds::Blinds(
    std::shared_ptr<SomfyRemotes> remotes,
    std::shared_ptr<DeviceConfig> config,
    std::shared_ptr<MqttClient> mqttClient,
    std::shared_ptr<WebServer> webServer)
:   _remotes(std::move(remotes)),
    _config(std::move(config)),
    _mqttClient(std::move(mqttClient)),
    _webServer(webServer),
    _addBlindSubscription(_webServer, 0, "addblind", [this](const uint8_t *payload, uint32_t length) { DoAddBlind(payload, length); }),
    _deleteBlindSubscription(_webServer, 0, "deleteblind", [this](const uint8_t *payload, uint32_t length) { DoDeleteBlind(payload, length); }),
    _nextId(1)
{

    uint32_t count;
    const uint16_t *blindids = _config->GetBlindIds(&count);


    for(auto a = 0; a < count; a++)
    {
        if(blindids[a] >= _nextId)
            _nextId = blindids[a] + 1;
        auto cfg = _config->GetBlindConfig(blindids[a]);
        _blinds.insert({blindids[a], std::make_unique<Blind>(blindids[a], cfg, remotes->GetRemote(cfg->remoteId), _mqttClient, _webServer)});
    }
}


void Blinds::DoAddBlind(const uint8_t *payload, uint32_t length)
{
    // Create a new blind, and a new remote for the blind
    auto newRemote = _remotes->CreateRemote();
    auto newId = _nextId++;

    BlindConfig cfg;
    strncpy(cfg.blindName, payload);
    cfg.currentPosition = 100;
    cfg.myPosition = 50;
    cfg.

    _blinds.insert({newId, std::make_unique<Blind>(newId, 
}

void Blinds::DoDeleteBlind(const uint8_t *payload, uint32_t length)
{
    // TODO!
}


