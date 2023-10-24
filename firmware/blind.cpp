// Copyright (c) 2023 Mark Godwin.
// SPDX-License-Identifier: MIT

#include "pico/stdlib.h"

#include "blind.h"
#include "remote.h"
#include <string.h>
#include <math.h>
#include "deviceConfig.h"
#include "bufferOutput.h"



Blind::Blind(
    uint16_t blindId,
    std::string name,
    int currentPosition,
    int favouritePosition,
    int openTime,
    int closeTime,
    std::shared_ptr<SomfyRemote> remote,
    std::shared_ptr<MqttClient> mqttClient,
    std::shared_ptr<DeviceConfig> config)
:   _blindId(blindId),
    _isDirty(false),
    _needsPublish(true),
    _name(std::move(name)),
    _targetPosition(currentPosition),
    _intermediatePosition(currentPosition),
    _openTime(openTime),
    _closeTime(closeTime),
    _remote(remote),
    _mqttClient(mqttClient),
    _favouritePosition(favouritePosition),
    _config(config),
    _motionDirection(0),
    _cmdSubscription(mqttClient, string_format("pico_somfy/blinds/%08x/cmd", blindId),[this](const uint8_t *payload, uint32_t length) { OnCommand(payload, length); }),
    _posSubscription(mqttClient, string_format("pico_somfy/blinds/%08x/pos", blindId),[this](const uint8_t *payload, uint32_t length) { OnSetPosition(payload, length); }),
    _refreshTimer([this]() { return UpdatePosition(); }, 0),
    _discoveryWorker([this]() { PublishDiscovery(); })
{
    printf("Blind ID %d: %s\n", _blindId, _name.c_str());
    printf("    Target Position: %d\nOpen time: %d\nClose time: %d\nFavourite: %d", _targetPosition, _openTime, _closeTime, _favouritePosition);

    if(_favouritePosition > 100 || _favouritePosition < 0)
        _favouritePosition = 50;

    _lastTick = get_absolute_time();
}

Blind::~Blind()
{
}

void Blind::UpdateConfig(std::string name, int openTime, int closeTime)
{
    _name = std::move(name);
    _openTime = openTime;
    _closeTime = closeTime;
    _isDirty = true;
    _remote->SetName(_name);    // Keep the name of the primary remote in sync.
    TriggerPublishDiscovery();
}

uint32_t Blind::GetRemoteId()
{
    return _remote->GetRemoteId(); 
}

void Blind::GoToPosition(int position)
{
    if(position > 100)
        position = 100;
    else if(position < 0)
        position = 0;
    if(_intermediatePosition > position || position == 0)
        _remote->PressButtons(SomfyButton::Down, ShortPress);
    if(_intermediatePosition < position || position == 100)
        _remote->PressButtons(SomfyButton::Up, ShortPress);

    _targetPosition = position;
}

void Blind::GoUp()
{
    _remote->PressButtons(SomfyButton::Up, ShortPress);
}

void Blind::GoDown()
{
    _remote->PressButtons(SomfyButton::Down, ShortPress);
}

void Blind::Stop()
{
    if(_motionDirection)
        _remote->PressButtons(SomfyButton::My, ShortPress);
}

void Blind::GoToMyPosition()
{
    if(_motionDirection)
        return;
    _remote->PressButtons(SomfyButton::My, ShortPress);
}

void Blind::ButtonsPressed(SomfyButton button)
{
    if(button == SomfyButton::Up)
    {
        _targetPosition = 100;
        _motionDirection = 1;
    }
    if(button == SomfyButton::Down)
    {
        _targetPosition = 0;
        _motionDirection = -1;
    }
    if(button == SomfyButton::My)
    {
        if(_motionDirection)
        {
            _targetPosition = round(_intermediatePosition);
            _motionDirection = 0;
        }
        else
        {
            _targetPosition = _favouritePosition;
            _motionDirection = _targetPosition > _intermediatePosition ? 1 : -1;
        }
    }

    _lastTick = get_absolute_time();
     _refreshTimer.ResetTimer(500);
}

void Blind::SaveMyPosition()
{
    if(!_motionDirection)
    {
        _remote->PressButtons(SomfyButton::My, LongPress);
        _favouritePosition = _targetPosition;
    }
}

void Blind::SaveConfig(bool force)
{
    if(!_isDirty && !force)
    {
        return;
    }

    BlindConfig config;
    strcpy(config.blindName, _name.c_str());
    config.closeTime = _closeTime;
    config.openTime = _openTime;
    config.remoteId = _remote->GetRemoteId();
    config.currentPosition = _targetPosition;
    config.myPosition = _favouritePosition;

    printf("Saving blind ID %d: %s\n", _blindId, config.blindName);
    printf("    Primary Remote: %08x\n    Position: %d\n    Open time: %d\n", config.remoteId, config.currentPosition, config.openTime);

    _config->SaveBlindConfig(_blindId, &config);
    _isDirty = false;
}

uint32_t Blind::UpdatePosition()
{
    // Tick - update position, motion state & publish
    auto now = get_absolute_time();
    auto elapsed = absolute_time_diff_us(_lastTick, now) / 1000000.0f;
    printf("Blind %d, TargetPosition %d, IntermediatePosition %hf, Tick: %hfs\n", _blindId, _targetPosition, _intermediatePosition, elapsed);

    if(_motionDirection)
    {
        auto pctPerSecond = 100.0f / (_motionDirection > 0 ? _openTime : -_closeTime);
        // Update the blind position, given the time elapsed
        auto travelled = elapsed * pctPerSecond;
        _intermediatePosition += travelled;
        if(_intermediatePosition > 100)
            _intermediatePosition = 100;
        else if(_intermediatePosition < 0)
            _intermediatePosition = 0;

        // We will have stopped if we have reached or passed our target
        if(_motionDirection > 0 && _intermediatePosition >= _targetPosition ||
            _motionDirection < 0 && _intermediatePosition <= _targetPosition)
        {
            _intermediatePosition = _targetPosition;
            if(_targetPosition < 100 && _targetPosition > 0)
                Stop();
            _motionDirection = 0;
        }
    }

    _lastTick = now;

    // Tell the MQTT subscribers where we are at
    PublishPosition();

    // Tick again if we're in motion
    return _motionDirection ? 1000 : 0;
}

void Blind::OnCommand(const uint8_t *payload, uint32_t length)
{
    if(length == 4 && !memcmp(payload, "open", 4))
        GoUp();
    else if(length == 5 && !memcmp(payload, "close", 5))
        GoDown();
    else if(length == 4 && !memcmp(payload, "stop", 4))
        Stop();
}

void Blind::OnSetPosition(const uint8_t *payload, uint32_t length)
{
    if(length > 31)
        return;
    char payloadstr[32];
    memcpy(payloadstr, payload, length);
    payloadstr[length] = 0;
    float position = 0;
    sscanf(payloadstr, "%f", &position);
    int pos = round(position);

    GoToPosition(pos);
}


void Blind::PublishPosition()
{
    char topic[42];
    sprintf(topic, "pico_somfy/blinds/%08x/position", _blindId);

    char buff[16];
    BufferOutput payload(buff, sizeof(buff));
    payload.Append((int)_intermediatePosition);
    _mqttClient->Publish(topic, (uint8_t *)buff, payload.BytesWritten());

    sprintf(topic, "pico_somfy/blinds/%08x/state", _blindId);
    _mqttClient->Publish(
        topic,
        (uint8_t *)(_motionDirection > 0 ? "opening" :
                    _motionDirection < 0 ? "closing" :
                                           "stopped"),
        7); // All payloads are 7 bytes...
}

void Blind::PublishDiscovery()
{
    if(!_mqttClient->IsConnected())
        return;

    printf("Discovery publish for blind %d (%s)\n", _blindId, _name.c_str()); 
    auto mqttConfig = _config->GetMqttConfig();
    if(!*mqttConfig->topic)
    {
        puts("Discovery topic not configured");
        _needsPublish = false;
        return;
    }
    printf("Discovery topic: %s\n", mqttConfig->topic); 

    char payload[512];
    BufferOutput payloadWriter(payload, sizeof(payload));
    
    payloadWriter.Append("{ \"~\": \"pico_somfy/blinds/");
    payloadWriter.AppendHex(_blindId);
    payloadWriter.Append("\", \"name\": null");
    //payloadWriter.AppendEscaped(this->_name.c_str());
    payloadWriter.Append(
        ", \"avty_t\": \"pico_somfy/status\", \"pl_avail\": \"online\", \"pl_not_avail\": \"offline\", "
        "\"stat_t\": \"~/state\", \"cmd_t\": \"~/cmd\", \"pl_open\": \"open\", \"pl_cls\": \"close\", \"pl_stop\": \"stop\", "
        "\"pos_t\": \"~/position\", \"set_pos_t\": \"~/pos\", \"uniq_id\": \"ps_cover_");
    payloadWriter.AppendHex(_blindId);
    payloadWriter.Append("\", \"device\": { \"name\": \"");
    payloadWriter.AppendEscaped(this->_name.c_str());
    payloadWriter.Append("\", \"mdl\": \"Pico-Somfy controlled cover\", \"mf\": \"Bagpuss\", \"ids\": [\"psb_");
    payloadWriter.AppendHex(_blindId);
    payloadWriter.Append("\"] } }");

    char topic[64];
    snprintf(topic, sizeof(topic), "%s/cover/pico_somfy/%08x/config", mqttConfig->topic, _blindId);
    topic[63] = 0;
    printf("Publishing %d bytes to %s\n", payloadWriter.BytesWritten(), topic);
    if(_mqttClient->Publish(topic, (uint8_t *)payload, payloadWriter.BytesWritten()))
        _needsPublish = false;

}

