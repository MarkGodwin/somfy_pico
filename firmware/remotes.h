// Copyright (c) 2023 Mark Godwin.
// SPDX-License-Identifier: MIT

#pragma once

#include "remote.h"
#include <list>
#include <memory>
#include <map>
#include "webServer.h"
#include "scheduler.h"

class DeviceConfig;
class RadioCommandQueue;
class Blinds;
class MqttClient;

class SomfyRemotes
{
    public:
        SomfyRemotes(
            std::shared_ptr<DeviceConfig> config,
            std::shared_ptr<Blinds> blinds,
            std::shared_ptr<WebServer> webServer,
            std::shared_ptr<RadioCommandQueue> commandQueue,
            std::shared_ptr<MqttClient> mqttClient);

        std::shared_ptr<SomfyRemote> GetRemote(uint32_t remoteId);
        std::shared_ptr<SomfyRemote> CreateRemote(std::string remoteName);
        void DeleteRemote(uint32_t remoteId);

        /// @brief Publish discovery info for any devices that need to, now that Mqtt is connected
        /// @return True if there is more work to be done
        bool TryRepublish();
        void SaveRemoteState();

    private:

        void SaveRemoteList();

        std::shared_ptr<SomfyRemote> GetRemoteParam(const CgiParams &params);

        bool DoAddRemote(const CgiParams &params);
        bool DoUpdateRemote(std::shared_ptr<SomfyRemote> remote, const CgiParams &params);
        bool DoDeleteRemote(std::shared_ptr<SomfyRemote> remote, const CgiParams &params);
        bool DoAddOrRemoveBlindToRemote(std::shared_ptr<SomfyRemote> remote, const CgiParams &params, blindAssocFunc assocFunc);
        bool DoButtonPress(std::shared_ptr<SomfyRemote> remote, const CgiParams &params);

        uint16_t GetRemotesResponse(char *pcInsert, int iInsertLen, uint16_t tagPart, uint16_t *nextPart);

        std::shared_ptr<DeviceConfig> _config;
        std::shared_ptr<Blinds> _blinds;
        std::shared_ptr<RadioCommandQueue> _commandQueue;
        std::shared_ptr<MqttClient> _mqttClient;
        std::map<uint32_t, std::shared_ptr<SomfyRemote>> _remotes;

        uint32_t _nextId;

        std::list<CgiSubscription> _webApi;
        std::list<SsiSubscription> _webData;
        ScheduledTimer _saveTimer;
};
