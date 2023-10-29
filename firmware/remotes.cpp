// Copyright (c) 2023 Mark Godwin.
// SPDX-License-Identifier: MIT

#include "picoSomfy.h"

#include "remotes.h"
#include "deviceConfig.h"
#include "blinds.h"
#include "bufferOutput.h"

const uint32_t BaseRemoteId = 0x270000;

SomfyRemotes::SomfyRemotes(
    std::shared_ptr<DeviceConfig> config,
    std::shared_ptr<Blinds> blinds,
    std::shared_ptr<WebServer> webServer,
    std::shared_ptr<RadioCommandQueue> commandQueue,
    std::shared_ptr<MqttClient> mqttClient)
:   _config(std::move(config)),
    _blinds(std::move(blinds)),
    _commandQueue(std::move(commandQueue)),
    _mqttClient(mqttClient),
    _saveTimer([this]() { SaveRemoteState(); return SAVE_DELAY; }, SAVE_DELAY)    // Potentially save every two minutes, to avoid writing rolling code changes too often when lots of presses happen
{

    uint32_t count;
    const uint16_t *remoteIds = _config->GetRemoteIds(&count);

    DBG_PRINT("There are %d remotes registered\n", count);

    _nextId = BaseRemoteId + 1;
    for(auto a = 0; a < count; a++)
    {
        auto id = remoteIds[a] | BaseRemoteId;
        if(id >= _nextId)
            _nextId = id + 1;
        auto cfg = _config->GetRemoteConfig(id);
        if(cfg)
        {
            std::vector<uint16_t> assocBlinds(cfg->blinds, cfg->blinds + cfg->blindCount);
            auto remote = std::make_shared<SomfyRemote>(_commandQueue, _blinds, _config, _mqttClient, cfg->remoteName, id, cfg->rollingCode, assocBlinds, false);
            _remotes.insert({id, remote});
        }
        else
        {
            DBG_PRINT("Missing remote: %x\n", id);
        }
    }

    const uint32_t *externalRemtoeIds = _config->GetExternalRemoteIds(&count);
    DBG_PRINT("There are %d external remotes registered\n", count);
    for(auto a = 0; a < count; a++)
    {
        auto id = externalRemtoeIds[a];
        auto cfg = _config->GetRemoteConfig(id);
        if(cfg)
        {
            std::vector<uint16_t> assocBlinds(cfg->blinds, cfg->blinds + cfg->blindCount);
            auto remote = std::make_shared<SomfyRemote>(_commandQueue, _blinds, _config, _mqttClient, cfg->remoteName, id, cfg->rollingCode, assocBlinds, true);
            _remotes.insert({id, remote});
        }
        else
        {
            DBG_PRINT("Missing external remote: %x\n", id);
        }

    }

    _webApi.push_back(CgiSubscription(webServer, "/api/remotes/add.json", [this](const CgiParams &params) { return DoAddRemote(params); }));
    _webApi.push_back(CgiSubscription(webServer, "/api/remotes/import.json", [this](const CgiParams &params) { return DoImportRemote(params); }));
    _webApi.push_back(CgiSubscription(webServer, "/api/remotes/update.json", [this](const CgiParams &params) { return DoUpdateRemote(GetRemoteParam(params), params); }));
    _webApi.push_back(CgiSubscription(webServer, "/api/remotes/delete.json", [this](const CgiParams &params) { return DoDeleteRemote(GetRemoteParam(params), params); }));
    _webApi.push_back(CgiSubscription(webServer, "/api/remotes/bindBlind.json", [this](const CgiParams &params) { return DoAddOrRemoveBlindToRemote(GetRemoteParam(params), params, &SomfyRemote::AssociateBlind); }));
    _webApi.push_back(CgiSubscription(webServer, "/api/remotes/unbindBlind.json", [this](const CgiParams &params) { return DoAddOrRemoveBlindToRemote(GetRemoteParam(params), params, &SomfyRemote::DisassociateBlind); }));
    _webApi.push_back(CgiSubscription(webServer, "/api/remotes/command.json", [this](const CgiParams &params) { return DoButtonPress(GetRemoteParam(params), params); }));
    _webApi.push_back(CgiSubscription(webServer, "/api/remotes/discover.json", [this](const CgiParams &params) { return StartDiscovery(); }));

    _webData.push_back(SsiSubscription(webServer, "remotes", [this](char *buffer, int len, uint16_t tagPart, uint16_t *nextPart) { return GetRemotesResponse(buffer, len, tagPart, nextPart); }));
    _webData.push_back(SsiSubscription(webServer, "discov", [this](char *buffer, int len, uint16_t tagPart, uint16_t *nextPart) { return GetDiscoveryResponse(buffer, len, tagPart, nextPart); }));

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
    DBG_PRINT("Creating new remote with id %08x\n", _nextId);
    auto id = _nextId++;
    auto newRemote = std::make_shared<SomfyRemote>(_commandQueue, _blinds, _config, _mqttClient, remoteName, id, 1, std::vector<uint16_t>(), false);
    _remotes.insert({id, newRemote});
    newRemote->SaveConfig(true);

    SaveRemoteList();
    return newRemote;
}

void SomfyRemotes::DeleteRemote(uint32_t remoteId)
{
    _remotes.erase(remoteId);
    _config->DeleteRemoteConfig(remoteId);
    SaveRemoteList();
}

bool SomfyRemotes::TryRepublish()
{
    for(auto iter = _remotes.begin(); iter != _remotes.end(); iter++)
    {
        if(iter->second->NeedsPublish())
        {
            iter->second->TriggerPublishDiscovery();
            return true;
        }
    }

    // No remotes needed publishing
    return false;
}

void SomfyRemotes::SaveRemoteState()
{
    for(auto iter = _remotes.begin(); iter != _remotes.end(); iter++)
    {
        iter->second->SaveConfig();
    }
}

void SomfyRemotes::ExternalButtonPress(SomfyCommand command)
{
    auto entry = _remotes.find(command.remoteId);
    if(entry == _remotes.end())
    {
        // A command from a remote we don't know. So remember the ID for discovery later
        _detectedRemotes.push_back(command.remoteId);
        if(_detectedRemotes.size() > 4)
            _detectedRemotes.pop_back();
    }
    else
    {
        entry->second->ExternalButtonPress(command.button, command.repeat, command.rollingCode);
    }

}

void SomfyRemotes::SaveRemoteList()
{
    {
        uint16_t ids[128];
        uint16_t *idoff = ids;
        for(auto iter = _remotes.begin(); iter != _remotes.end(); iter++)
        {
            if(!iter->second->IsExternal())
                *idoff++ = iter->first & 0xFFFF;
        }
        _config->SaveRemoteIds(ids, idoff - ids);
    }

    {
        uint32_t ids[64];
        uint32_t *idoff = ids;
        for(auto iter = _remotes.begin(); iter != _remotes.end(); iter++)
        {
            if(iter->second->IsExternal())
                *idoff++ = iter->first;
        }
        _config->SaveExternalRemoteIds(ids, idoff - ids);
    }
}

std::shared_ptr<SomfyRemote> SomfyRemotes::GetRemoteParam(const CgiParams &params)
{
    auto idparam = params.find("id");
    uint32_t id;
    if(idparam == params.end() ||
        !sscanf(idparam->second.c_str(), "%u", &id))
    {
        DBG_PUT("Invalid command: Bad id parameter\n");
        return nullptr;
    }

    auto entry = _remotes.find(id);
    if(entry == _remotes.end())
    {
        DBG_PRINT("Invalid command: No remote with id %d\n", id);
        return nullptr;
    }
    return entry->second;
}

bool SomfyRemotes::DoAddRemote(const CgiParams &params)
{
    auto nameparam = params.find("name");
    if(nameparam == params.end() ||
        !nameparam->second.length() ||
        nameparam->second.length() + 1 > sizeof(RemoteConfig::remoteName) ||
        nameparam->second.find('\"') != std::string::npos)
    {
        DBG_PUT("Bad name parameter\n");
        return false;
    }

    CreateRemote(nameparam->second);
    return true;
}

bool SomfyRemotes::DoImportRemote(const CgiParams &params)
{
    auto nameparam = params.find("name");
    if(nameparam == params.end() ||
        !nameparam->second.length() ||
        nameparam->second.length() + 1 > sizeof(RemoteConfig::remoteName) ||
        nameparam->second.find('\"') != std::string::npos)
    {
        DBG_PUT("Bad name parameter\n");
        return false;
    }

    auto idparam = params.find("id");
    uint32_t id;
    if(idparam == params.end() ||
        !sscanf(idparam->second.c_str(), "%u", &id))
    {
        DBG_PUT("Invalid command: Bad id parameter\n");
        return false;
    }

    if(_remotes.find(id) != _remotes.end())
    {
        DBG_PUT("Invalid command: External remote already exists\n");
        return false;
    }

    DBG_PRINT("Importing new remote with id %08x\n", _nextId);
    auto newRemote = std::make_shared<SomfyRemote>(_commandQueue, _blinds, _config, _mqttClient, nameparam->second, id, 1, std::vector<uint16_t>(), true);
    _remotes.insert({id, newRemote});
    newRemote->SaveConfig(true);
    SaveRemoteList();
    return true;
}

bool SomfyRemotes::DoUpdateRemote(std::shared_ptr<SomfyRemote> remote, const CgiParams &params)
{
    if(!remote)
        return false;

    auto nameparam = params.find("name");
    if(nameparam == params.end() ||
        !nameparam->second.length() ||
        nameparam->second.length() + 1 > sizeof(RemoteConfig::remoteName) ||
        nameparam->second.find('\"') != std::string::npos)
    {
        DBG_PUT("Bad name parameter\n");
        return false;
    }

    remote->SetName( nameparam->second);
    return true;
}

bool SomfyRemotes::DoDeleteRemote(std::shared_ptr<SomfyRemote> remote, const CgiParams &params)
{
    // Can't remove the primary remote for any blind - we should remove the blind instead
    auto remoteId = remote->GetRemoteId();
    if(_blinds->IsAPrimaryRemote(remoteId))
        return false;

    DeleteRemote(remoteId);
    return true;
}


bool SomfyRemotes::DoAddOrRemoveBlindToRemote(std::shared_ptr<SomfyRemote> remote, const CgiParams &params, blindAssocFunc assocFunc)
{
    if(!remote)
        return false;

    auto idparam = params.find("blindId");
    uint16_t id;
    if(idparam == params.end() ||
        !sscanf(idparam->second.c_str(), "%hu", &id))
    {
        DBG_PUT("Invalid command: Bad blindId parameter\n");
        return false;
    }

    if(!_blinds->Exists(id))
    {
        DBG_PRINT("No blind exists with id %d\n", id);
        return false;
    }

    ((*remote).*assocFunc)(id);
    return true;
}

bool SomfyRemotes::DoButtonPress(std::shared_ptr<SomfyRemote> remote, const CgiParams &params)
{
    if(!remote)
        return false;

    auto buttonParam = params.find("buttons");
    SomfyButton buttons;
    if(buttonParam == params.end() ||
        !sscanf(buttonParam->second.c_str(), "%d", &buttons))
    {
        DBG_PUT("Invalid command: Bad buttons parameter\n");
    }
    auto longParam = params.find("long");
    auto longPress = (longParam != params.end() && longParam->second == "true");

    remote->PressButtons(buttons, longPress ? LongPress : ShortPress);
    return true;
}

bool SomfyRemotes::StartDiscovery()
{
    _detectedRemotes.clear();
    return true;
}

uint16_t SomfyRemotes::GetRemotesResponse(char *pcInsert, int iInsertLen, uint16_t tagPart, uint16_t *nextPart)
{
    // This data structure is a poor choice for this API
    auto pos = _remotes.begin();
    for(auto a = 0; a < tagPart; a++)
    {
        pos++;
    }

    if(pos == _remotes.end())
        return 0;

    BufferOutput outputter(pcInsert, iInsertLen);
    outputter.Append("{ \"id\": ");
    outputter.Append((int)pos->first);
    outputter.Append(", \"name\": \"");
    outputter.Append(pos->second->GetName());
    outputter.Append("\", \"blinds\": [");

    bool first = true;
    for(auto blindId :pos->second->GetAssociatedBlinds())
    {
        if(first)
            first = false;
        else
            outputter.Append(',');
        outputter.Append(blindId);
    }
    outputter.Append("],\"external\":");
    outputter.Append(pos->second->IsExternal()?"true":"false");

    pos++;
    if(pos != _remotes.end())
    {
        outputter.Append(" },");
        *nextPart = tagPart + 1;
    }
    else
        outputter.Append(" }");
    
    return outputter.BytesWritten();
}

uint16_t SomfyRemotes::GetDiscoveryResponse(char *pcInsert, int iInsertLen, uint16_t tagPart, uint16_t *nextPart)
{
    BufferOutput outputter(pcInsert, iInsertLen);
    bool first = true;
    for(auto iter = _detectedRemotes.begin(); iter != _detectedRemotes.end(); iter++)
    {
        if(first)
            first = false;
        else
            outputter.Append(',');
        outputter.Append((int)(*iter));
    }
    return outputter.BytesWritten();
}
