#include "pico/stdlib.h"

#include "remotes.h"
#include "deviceConfig.h"
#include "blinds.h"

const uint32_t BaseRemoteId = 0x270000;

SomfyRemotes::SomfyRemotes(
    std::shared_ptr<DeviceConfig> config,
    std::shared_ptr<Blinds> blinds,
    std::shared_ptr<WebServer> webServer,
    std::shared_ptr<RadioCommandQueue> commandQueue)
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

    _webApi.push_back(CgiSubscription(webServer, "/api/remotes/add.json", [this](const CgiParams &params) { return DoAddRemote(params); }));
    _webApi.push_back(CgiSubscription(webServer, "/api/remotes/update.json", [this](const CgiParams &params) { return DoUpdateRemote(GetRemoteParam(params), params); }));
    _webApi.push_back(CgiSubscription(webServer, "/api/remotes/delete.json", [this](const CgiParams &params) { return DoDeleteRemote(GetRemoteParam(params), params); }));
    _webApi.push_back(CgiSubscription(webServer, "/api/remotes/addBlind.json", [this](const CgiParams &params) { return DoAddOrRemoveBlindToRemote(GetRemoteParam(params), params, &SomfyRemote::AssociateBlind); }));
    _webApi.push_back(CgiSubscription(webServer, "/api/remotes/removeBlind.json", [this](const CgiParams &params) { return DoAddOrRemoveBlindToRemote(GetRemoteParam(params), params, &SomfyRemote::DisassociateBlind); }));
    _webApi.push_back(CgiSubscription(webServer, "/api/remotes/command.json", [this](const CgiParams &params) { return DoButtonPress(GetRemoteParam(params), params); }));

    _webData.push_back(SsiSubscription(webServer, "remotes", [this](char *buffer, int len, uint16_t tagPart, uint16_t *nextPart) { return GetRemotesResponse(buffer, len, tagPart, nextPart); }));

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

std::shared_ptr<SomfyRemote> SomfyRemotes::GetRemoteParam(const CgiParams &params)
{
    auto idparam = params.find("id");
    uint32_t id;
    if(idparam == params.end() ||
        !sscanf(idparam->second.c_str(), "%u", &id))
    {
        puts("Invalid command: Bad id parameter\n");
        return nullptr;
    }

    auto entry = _remotes.find(id);
    if(entry == _remotes.end())
    {
        printf("Invalid command: No remote with id %d\n", id);
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
        puts("Bad name parameter\n");
        return false;
    }

    auto id = _nextId++;
    _remotes.insert({id, std::make_shared<SomfyRemote>(_commandQueue, _blinds, nameparam->second, id, 1, std::vector<uint16_t>())});

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
        puts("Bad name parameter\n");
        return false;
    }

    remote->SetName( nameparam->second);
    return true;
}

bool SomfyRemotes::DoDeleteRemote(std::shared_ptr<SomfyRemote> remote, const CgiParams &params)
{
    // TODO!
    return false;
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
        puts("Invalid command: Bad blindId parameter\n");
        return false;
    }

    if(!_blinds->Exists(id))
    {
        printf("No blind exists with id %d\n", id);
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
        puts("Invalid command: Bad buttons parameter\n");
    }
    auto longParam = params.find("long");
    auto longPress = (longParam != params.end() && longParam->second == "true");

    remote->PressButtons(buttons, longPress ? LongPress : ShortPress);
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
    outputter.Append(']');
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
