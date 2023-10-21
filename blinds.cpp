

#include "pico/stdlib.h"
#include <string.h>

#include "blinds.h"
#include "deviceConfig.h"
#include "remotes.h"
#include "bufferOutput.h"

Blinds::Blinds(
    std::shared_ptr<DeviceConfig> config,
    std::shared_ptr<MqttClient> mqttClient,
    std::shared_ptr<WebServer> webServer)
:   _config(std::move(config)),
    _mqttClient(std::move(mqttClient)),
    _webServer(webServer),
    _nextId(1),
    _saveTimer([this]() { SaveBlindState(false); return SAVE_DELAY; }, SAVE_DELAY)
{
}

void Blinds::Initialize(std::shared_ptr<SomfyRemotes> remotes)
{
    _remotes = remotes;

    uint32_t count;
    const uint16_t *blindids = _config->GetBlindIds(&count);

    printf("There are %d blinds registered\n", count);

    auto errors = false;
    for(auto a = 0; a < count; a++)
    {
        if(blindids[a] >= _nextId)
            _nextId = blindids[a] + 1;
        auto cfg = _config->GetBlindConfig(blindids[a]);
        auto remote = _remotes->GetRemote(cfg->remoteId);
        if(remote)
        {
            auto blind = std::make_unique<Blind>(blindids[a], cfg->blindName, cfg->currentPosition, cfg->myPosition, cfg->openTime, cfg->closeTime, remote, _mqttClient, _config);
            _blinds.insert(
                {
                    blindids[a],
                    std::move(blind)
                });
        }
        else
        {
            printf("Deleting blind id %d with missing remote %08x\n", blindids[a], cfg->remoteId );
            _config->DeleteBlindConfig(blindids[a]);
            errors = true;
        }
    }

    if(errors)
    {
        SaveBlindList();
    }

    _webApi.push_back(CgiSubscription(_webServer, "/api/blinds/add.json", [this](const CgiParams &params) { return DoAddBlind(params); }));
    _webApi.push_back(CgiSubscription(_webServer, "/api/blinds/update.json", [this](const CgiParams &params) { return DoUpdateBlind(params); }));
    _webApi.push_back(CgiSubscription(_webServer, "/api/blinds/delete.json", [this](const CgiParams &params) { return DoDeleteBlind(params); }));
    _webApi.push_back(CgiSubscription(_webServer, "/api/blinds/command.json", [this](const CgiParams &params) { return DoBlindCommand(params); }));

    _webData.push_back(SsiSubscription(_webServer, "blinds", [this](char *buffer, int len, uint16_t tagPart, uint16_t *nextPart) { return GetBlindsResponse(buffer, len, tagPart, nextPart); }));
}

void Blinds::SaveBlindState(bool force)
{
    for(auto iter = _blinds.begin(); iter != _blinds.end(); iter++)
    {
        iter->second->SaveConfig(force);
    }
}

void Blinds::SaveBlindList()
{
    printf("Saving blind list with %d blind ids.\n", _blinds.size());
    uint16_t ids[128];
    uint16_t *idoff = ids;
    for(auto iter = _blinds.begin(); iter != _blinds.end(); iter++)
    {
        *idoff++ = iter->first & 0xFFFF;
    }
    _config->SaveBlindIds(ids, _blinds.size());
}

bool Blinds::DoAddBlind(const CgiParams &params)
{
    auto nameParam = params.find("name");
    auto openTimeParam = params.find("openTime");
    auto closeTimeParam = params.find("closeTime");

    if(nameParam == params.end() ||
       openTimeParam == params.end() ||
       closeTimeParam == params.end())
    {
        puts("Missing arguments to AddBlind");
        return false;
    }

    int openTime, closeTime;
    if( !sscanf(openTimeParam->second.c_str(), "%d", &openTime) ||
        !sscanf(closeTimeParam->second.c_str(), "%d", &closeTime))
    {
        puts("Bad open or close time parameters");
        return false;
    }

    // Create a new blind, and a new remote for the blind
    auto newRemote = _remotes->CreateRemote(nameParam->second);
    auto newId = _nextId++;

    auto newBlind = std::make_unique<Blind>(newId, nameParam->second, 90, 50, openTime, closeTime, newRemote, _mqttClient, _config);
    newBlind->SaveConfig(true);

    auto created = _blinds.insert({newId, std::move(newBlind)});
    newRemote->AssociateBlind(newId);

    SaveBlindList();

    return true;
}

bool Blinds::DoUpdateBlind(const CgiParams &params)
{
    auto idParam = params.find("id");
    auto nameParam = params.find("name");
    auto openTimeParam = params.find("openTime");
    auto closeTimeParam = params.find("closeTime");

    if(idParam == params.end() ||
       nameParam == params.end() ||
       openTimeParam == params.end() ||
       closeTimeParam == params.end())
    {
        puts("Missing arguments to AddBlind");
        return false;
    }

    uint16_t id;
    int openTime, closeTime;
    if( !sscanf(idParam->second.c_str(), "%hu", &id) ||
        !sscanf(openTimeParam->second.c_str(), "%d", &openTime) ||
        !sscanf(closeTimeParam->second.c_str(), "%d", &closeTime))
    {
        puts("Bad open or close time parameters");
        return false;
    }

    auto pos = _blinds.find(id);
    if(pos == _blinds.end())
    {
        printf("No blind with id %d found\n", id);
        return false;
    }

    puts("Updating blind....");
    pos->second->UpdateConfig(nameParam->second, openTime, closeTime);
    return true;
}



bool Blinds::DoDeleteBlind(const CgiParams &params)
{
    auto idParam = params.find("id");
    uint32_t id;
    if(!sscanf(idParam->second.c_str(), "%d", &id))
    {
        printf("Can't read ID %s\n", idParam->second.c_str());
        return false;
    }
    
    // Unregister the remote from the blind
    auto removed = _blinds.extract(id);
    if(removed.empty())
    {
        return false;
    }

    auto remoteId = removed.mapped()->GetRemoteId();
    _remotes->DeleteRemote(remoteId);

    _config->DeleteBlindConfig(id);
    SaveBlindList();
    return false;
}

bool Blinds::DoBlindCommand(const CgiParams &params)
{
    auto idParam = params.find("id");
    auto commandParam = params.find("command");
    auto payloadParam = params.find("payload");

    if(idParam == params.end() || commandParam == params.end() || payloadParam == params.end())
    {
        puts("Bad command received - missing arguments\n");
        return false;
    }

    uint32_t id;
    if(!sscanf(idParam->second.c_str(), "%d", &id))
    {
        printf("Can't read ID %s\n", idParam->second.c_str());
        return false;
    }

    auto blindEntry = _blinds.find(id);
    if(blindEntry == _blinds.end())
    {
        printf("No blind found with ID %d\n", id);
        return false;
    }

    if(commandParam->second == "cmd")
        blindEntry->second->OnCommand((const uint8_t *)payloadParam->second.c_str(), payloadParam->second.length());
    else if(commandParam->second == "pos")
        blindEntry->second->OnSetPosition((const uint8_t *)payloadParam->second.c_str(), payloadParam->second.length());
    else
        printf("Unexpected command: %s", commandParam->second.c_str());
    return true;
}

uint16_t Blinds::GetBlindsResponse(char *pcInsert, int iInsertLen, uint16_t tagPart, uint16_t *nextPart)
{
    // This data structure is a poor choice for this API
    auto pos = _blinds.begin();
    for(auto a = 0; a < tagPart; a++)
    {
        pos++;
    }

    if(pos == _blinds.end())
        return 0;

    BufferOutput outputter(pcInsert, iInsertLen);
    outputter.Append("{ \"id\": ");
    outputter.Append((int)pos->first);
    outputter.Append(", \"name\": \"");
    outputter.AppendEscaped(pos->second->GetName().c_str());
    outputter.Append("\", \"position\": ");
    outputter.Append(pos->second->GetIntermediatePosition());
    outputter.Append(", \"openTime\": ");
    outputter.Append(pos->second->GetOpenTime());
    outputter.Append(", \"closeTime\": "); 
    outputter.Append(pos->second->GetCloseTime());
    outputter.Append(", \"remoteId\": ");
    outputter.Append((int)pos->second->GetRemoteId());
    outputter.Append(", \"state\": \"");
    auto md = pos->second->GetMotionDirection();
    outputter.Append(
        md > 0 ? "opening" :
        md < 0 ? "closing" :
        "stopped");

    pos++;
    if(pos != _blinds.end())
    {
        outputter.Append("\" },");
        *nextPart = tagPart + 1;
    }
    else
        outputter.Append("\" }");
    
    return outputter.BytesWritten();
}

bool Blinds::IsAPrimaryRemote(uint32_t remoteId)
{
    for(auto iter = _blinds.begin(); iter != _blinds.end(); iter++)
    {
        if(iter->second->GetRemoteId() == remoteId)
            return true;
    }
    return false;
}
