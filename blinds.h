
#pragma once

#include "blind.h"
#include <list>
#include <memory>
#include "webInterface.h"


class MqttClient;
class SomfyRemotes;


class Blinds
{
    public:
        Blinds(
            std::shared_ptr<SomfyRemotes> remotes,
            std::shared_ptr<DeviceConfig> config,
            std::shared_ptr<MqttClient> mqttClient,
            std::shared_ptr<WebServer> webServer);

    private:
        void DoAddBlind(const uint8_t *payload, uint32_t length);
        void DoDeleteBlind(const uint8_t *payload, uint32_t length);

        uint32_t _nextId;
        std::map<uint32_t, std::unique_ptr<Blind>> _blinds;

        std::shared_ptr<DeviceConfig> _config;
        std::shared_ptr<SomfyRemotes> _remotes;
        std::shared_ptr<MqttClient> _mqttClient;
        std::shared_ptr<WebServer> _webServer;

        CgiSubscription _addBlindSubscription;
        CgiSubscription _deleteBlindSubscription;
};

