
#pragma once

#include "blind.h"
#include <list>
#include <memory>
#include "webInterface.h"
#include "scheduler.h"

class MqttClient;
class SomfyRemotes;


class Blinds
{
    public:
        Blinds(
            std::shared_ptr<DeviceConfig> config,
            std::shared_ptr<MqttClient> mqttClient,
            std::shared_ptr<WebServer> webServer);

        void Initialize(std::shared_ptr<SomfyRemotes> remotes);

        const std::unique_ptr<Blind> &GetBlind(uint16_t blindId)
        {
            return _blinds.at(blindId);
        }

        bool Exists(uint16_t blindId)
        {
            return _blinds.count(blindId) > 0;
        }

        bool IsAPrimaryRemote(uint32_t remoteId);

        void SaveBlindState(bool force = false);

    private:

        void SaveBlindList();

        bool DoAddBlind(const CgiParams &params);
        bool DoUpdateBlind(const CgiParams &params);
        bool DoDeleteBlind(const CgiParams &params);
        bool DoBlindCommand(const CgiParams &params);

        uint16_t GetBlindsResponse(char *pcInsert, int iInsertLen, uint16_t tagPart, uint16_t *nextPart);

        uint32_t _nextId;
        std::map<uint32_t, std::unique_ptr<Blind>> _blinds;

        std::shared_ptr<DeviceConfig> _config;
        std::shared_ptr<SomfyRemotes> _remotes;
        std::shared_ptr<MqttClient> _mqttClient;
        std::shared_ptr<WebServer> _webServer;

        std::list<CgiSubscription> _webApi;
        std::list<SsiSubscription> _webData;
        ScheduledTimer _saveTimer;
};

