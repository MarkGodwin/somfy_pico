// Copyright (c) 2023 Mark Godwin.
// SPDX-License-Identifier: MIT

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
#include "bufferOutput.h"

SomfyRemote::SomfyRemote(
    std::shared_ptr<RadioCommandQueue> commandQueue,
    std::shared_ptr<Blinds> blinds,
    std::shared_ptr<DeviceConfig> config,
    std::shared_ptr<MqttClient> mqttClient,
    std::string name,
    uint32_t remoteId,
    uint16_t rollingCode,
    std::vector<uint16_t> associatedBlinds)
    : _commandQueue(std::move(commandQueue)),
      _blinds(std::move(blinds)),
      _config(std::move(config)),
      _mqttClient(mqttClient),
      _remoteName(std::move(name)),
      _remoteId(remoteId),
      _rollingCode(rollingCode),
      _associatedBlinds(std::move(associatedBlinds)),
      _isDirty(false),
      _needsPublish(true),
      _cmdSubscription(mqttClient, string_format("pico_somfy/remotes/%08x/cmd", remoteId), [this](const uint8_t *payload, uint32_t length)
                       { OnCommand(payload, length); }),
      _discoveryWorker([this]()
                       { PublishDiscovery(); })
{
    printf("Remote ID %08x: %s\n", remoteId, name.c_str());
    printf("    Rolling code: %d\n    Blind Count: %d\n", _rollingCode, _associatedBlinds.size());
}

void SomfyRemote::SaveConfig(bool force)
{
    if (!_isDirty && !force)
        return;

    RemoteConfig config;
    strcpy(config.remoteName, _remoteName.c_str());
    config.remoteId = _remoteId;
    config.rollingCode = _rollingCode;
    config.blindCount = _associatedBlinds.size();
    if (config.blindCount > sizeof(config.blinds) / sizeof(config.blinds[0]))
        config.blindCount = sizeof(config.blinds) / sizeof(config.blinds[0]);
    memcpy(config.blinds, _associatedBlinds.data(), sizeof(uint16_t) * config.blindCount);

    printf("Saving remote ID %d: %s\n", config.remoteId, config.remoteName);
    printf("    Rolling code: %d\n    Blind Count: %d\n", config.rollingCode, config.blindCount);

    _config->SaveRemoteConfig(_remoteId, &config);
    _isDirty = false;
}

void SomfyRemote::PressButtons(SomfyButton buttons, uint16_t repeat)
{
    _isDirty = true;
    _commandQueue->QueueCommand(_remoteId, _rollingCode++, buttons, repeat);

    // Now tell all our connected blinds that we've sent a command
    for (auto blindId : _associatedBlinds)
    {
        _blinds->GetBlind(blindId)->ButtonsPressed(buttons);
    }
}

void SomfyRemote::AssociateBlind(uint16_t blindId)
{
    _isDirty = true;
    if (std::find(_associatedBlinds.begin(), _associatedBlinds.end(), blindId) != _associatedBlinds.end())
        // Already present :/
        return;
    _associatedBlinds.push_back(blindId);
}

void SomfyRemote::DisassociateBlind(uint16_t blindId)
{
    _isDirty = true;
    auto pos = std::find(_associatedBlinds.begin(), _associatedBlinds.end(), blindId);
    if (pos == _associatedBlinds.end())
        return;

    _associatedBlinds.erase(pos);
}

void SomfyRemote::OnCommand(const uint8_t *payload, uint32_t length)
{
    if (length == 2 && !memcmp(payload, "up", 2))
        PressButtons(SomfyButton::Up, ShortPress);
    else if (length == 4 && !memcmp(payload, "down", 4))
        PressButtons(SomfyButton::Down, ShortPress);
    else if (length == 4 && !memcmp(payload, "stop", 4))
        PressButtons(SomfyButton::My, ShortPress);
}

void SomfyRemote::PublishDiscovery()
{
    if(!_mqttClient->IsConnected())
        // We'll be called again when MQTT connects
        return;

    printf("Discovery publish for remote %d (%s)\n", _remoteId, _remoteName.c_str());
    auto mqttConfig = _config->GetMqttConfig();
    if (!*mqttConfig->topic)
    {
        puts("Discovery topic not configured\n");
        _needsPublish = false;
        return;
    }
    // No discovery for the primary remote for any blind
    if (_blinds->IsAPrimaryRemote(_remoteId))
    {
        puts("Primary remote for blind not published\n");
        _needsPublish = false;
        return;
    }
    printf("Discovery topic: %s\n", mqttConfig->topic);

    if( PublishDiscovery("up", "Up Button", mqttConfig->topic) &&
        PublishDiscovery("down", "Down Button", mqttConfig->topic) &&
        PublishDiscovery("stop", "Stop Button", mqttConfig->topic))
    {
        _needsPublish = false;
    }
}

bool SomfyRemote::PublishDiscovery(const char *cmd, const char *name, const char *baseTopic)
{

    char payload[512];
    BufferOutput payloadWriter(payload, sizeof(payload));

    payloadWriter.Append("{ \"name\": \"");
    payloadWriter.Append(name);
    payloadWriter.Append(
        "\", \"avty_t\": \"pico_somfy/status\", \"pl_avail\": \"online\", \"pl_not_avail\": \"offline\", \"cmd_t\": \"pico_somfy/remotes/");
    payloadWriter.AppendHex(_remoteId);
    payloadWriter.Append("/cmd\", \"pl_prs\": \"");
    payloadWriter.Append(cmd);
    payloadWriter.Append("\", \"uniq_id\": \"psrem_");
    payloadWriter.AppendHex(_remoteId);
    payloadWriter.Append(cmd);
    payloadWriter.Append("\", \"device\": { \"name\": \"");

    payloadWriter.AppendEscaped(this->_remoteName.c_str());
    payloadWriter.Append("\", \"mdl\": \"Pico-Somfy Remote\", \"mf\": \"Bagpuss\", \"ids\": [\"psr_");
    payloadWriter.AppendHex(_remoteId);
    payloadWriter.Append("\"] } }");

    char topic[68];
    snprintf(topic, sizeof(topic), "%s/button/pico_somfy/%08x_%s/config", baseTopic, _remoteId, cmd);
    topic[67] = 0;
    printf("Publishing %d bytes to %s\n", payloadWriter.BytesWritten(), topic);

    return _mqttClient->Publish(topic, (uint8_t *)payload, payloadWriter.BytesWritten());
}