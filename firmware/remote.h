// Copyright (c) 2023 Mark Godwin.
// SPDX-License-Identifier: MIT

#pragma once

#include <memory>
#include <vector>
#include "mqttClient.h"

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
class DeviceConfig;

class SomfyRemote
{
public:
    SomfyRemote(
        std::shared_ptr<RadioCommandQueue> commandQueue,
        std::shared_ptr<Blinds> blinds,
        std::shared_ptr<DeviceConfig> config,
        std::shared_ptr<MqttClient> mqttClient,
        std::string name,
        uint32_t remoteId,
        uint16_t rollingCode,
        std::vector<uint16_t> associatedBlinds);

    const std::string &GetName() { return _remoteName; }
    uint32_t GetRemoteId() { return _remoteId; }
    const std::vector<uint16_t> &GetAssociatedBlinds() { return _associatedBlinds; }

    void SetName(std::string name)
    {
        _isDirty = true;
         _remoteName = std::move(name);
         TriggerPublishDiscovery();
    }
    void AssociateBlind(uint16_t blindId);
    void DisassociateBlind(uint16_t blindId);

    /// @brief Save the current config of the remote to flash memory, if needed
    void SaveConfig(bool force = false);

    // Press buttons on the controller. Note that buttons can be chorded.
    void PressButtons(SomfyButton buttons, uint16_t repeat);

    bool NeedsPublish() { return _needsPublish; }
    void TriggerPublishDiscovery() {
        if(_mqttClient->IsEnabled())
        {
            _needsPublish = true;
            _discoveryWorker.ScheduleWork();
        }
    }


private:
    void OnCommand(const uint8_t *payload, uint32_t length);
    void PublishDiscovery();
    bool PublishDiscovery(const char *cmd, const char *name, const char *baseTopic);

    std::shared_ptr<RadioCommandQueue> _commandQueue;
    std::shared_ptr<Blinds> _blinds;
    std::shared_ptr<DeviceConfig> _config;
    std::shared_ptr<MqttClient> _mqttClient;
    std::string _remoteName;
    uint32_t _remoteId;
    uint16_t _rollingCode;
    bool _isDirty; // Does need save?
    bool _needsPublish;
    std::vector<uint16_t> _associatedBlinds;

    MqttSubscription _cmdSubscription;
    PendingWorker _discoveryWorker;
};

typedef void (SomfyRemote::*blindAssocFunc)(uint16_t);
