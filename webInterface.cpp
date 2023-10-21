
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "pico/flash.h"
#include "lwip/apps/httpd.h"
#include <map>
#include <string>

#include "webInterface.h"
#include "deviceConfig.h"
#include "serviceControl.h"


// HTTPD callbacks aren't conducive to multiple instances
WebServer *_globalInstance;

WebServer::WebServer(std::shared_ptr<DeviceConfig> config, ServiceControl &service, std::shared_ptr<IWifiConnection> wifiConnection, std::shared_ptr<WifiScanner> wifiScanner, bool apMode)
:   _config(std::move(config)),
    _service(service),
    _wifiConnection(std::move(wifiConnection)),
    _wifiScanner(std::move(wifiScanner)),
    _apMode(apMode)
{
    _globalInstance = this;
}

struct CgiContext {
    WebServer *pThis;
    bool result;
};

extern "C" {
    // Called before processing any URL with a query string
    void httpd_cgi_handler(struct fs_file *file, const char* uri, int iNumParams,
                                char **pcParam, char **pcValue, void *connectionState)
    {
        CgiContext *ctx = (CgiContext *)connectionState;
        ctx->result = ctx->pThis->HandleRequest(file, uri, iNumParams, pcParam, pcValue);
    }

    // Called by httpd when a file is opened
    void *fs_state_init(struct fs_file *file, const char *name)
    {
        // Hack
        auto ctx = new CgiContext;
        ctx->pThis = _globalInstance;
        ctx->result = false;
        return ctx;
    }

    // Called by httpd when a file is closed
    void fs_state_free(struct fs_file *file, void *state)
    {
        delete (CgiContext *)state;
    }
}


 uint16_t WebServer::HandleResponseEntry(int tagIndex, char *pcInsert, int iInsertLen, uint16_t tagPart, uint16_t *nextPart, void *connectionState)
 {
    auto ctx = (CgiContext *)connectionState;
    return ctx->pThis->HandleResponse(tagIndex, pcInsert, iInsertLen, tagPart, nextPart, ctx->result);
 }

// Note, max tag len is 8 chars!
const char *tags[] = {
    "ssid",
    "ssidList",
    "result",
    "mqttAddr",
    "mqttPort",
    "mqttUser",
    "mqttTopi"
};

#define TAGINDEX_SSID 0
#define TAGINDEX_SSIDLIST 1
#define TAGINDEX_RESULT 2
#define TAGINDEX_MQTTADDR 3
#define TAGINDEX_MQTTPORT 4
#define TAGINDEX_MQTTUSER 5
#define TAGINDEX_MQTTTOPIC 6

 uint16_t WebServer::HandleResponse(int tagIndex, char *pcInsert, int iInsertLen, uint16_t tagPart, uint16_t *nextPart, bool cgiResult)
 {
    switch(tagIndex)
    {
        case TAGINDEX_SSID:
        {
            auto cfg = _config->GetWifiConfig();
            auto ssidLen = strnlen(cfg->ssid, sizeof(cfg->ssid));
            memcpy(pcInsert, cfg->ssid, ssidLen);
            return ssidLen;
        }
        case TAGINDEX_SSIDLIST:
        {
            if(tagPart == 0)
            {
                _wifiScanner->CollectResults();

                // Trigger another WiFi Scan for next time the page refreshes
                if(!_apMode)
                    _wifiScanner->TriggerScan();
            }

            auto &ssids = _wifiScanner->GetSsids();
            char *writeOff = pcInsert;
            if(tagPart >= ssids.size())
                return 0;

            *writeOff++ = '\"';
            auto len = ssids[tagPart].size();
            memcpy(writeOff, ssids[tagPart].data(), len);
            writeOff += len;
            *writeOff++ = '\"';
            if(tagPart + 1 != ssids.size())
            {
                *writeOff++ = ',';
                *nextPart = tagPart + 1;
            }

            return writeOff - pcInsert;

        }
        case TAGINDEX_RESULT:
            if(cgiResult)
            {
                memcpy(pcInsert, "true", 4);
                return 4;
            }
            memcpy(pcInsert, "false", 5);
            return 5;

        case TAGINDEX_MQTTADDR:
        {
            auto cfg = _config->GetMqttConfig();
            auto len = strnlen(cfg->brokerAddress, sizeof(cfg->brokerAddress));
            memcpy(pcInsert, cfg->brokerAddress, len);
            return len;
        }
        case TAGINDEX_MQTTPORT:
        {
            auto cfg = _config->GetMqttConfig();
            char buf[16];
            auto len = sprintf(buf, "%d", cfg->port);
            memcpy(pcInsert, buf, len);
            return len;
        }
        case TAGINDEX_MQTTUSER:
        {
            auto cfg = _config->GetMqttConfig();
            auto len = strnlen(cfg->username, sizeof(cfg->username));
            memcpy(pcInsert, cfg->username, len);
            return len;
        }
        case TAGINDEX_MQTTTOPIC:
        {
            auto cfg = _config->GetMqttConfig();
            auto len = strnlen(cfg->topic, sizeof(cfg->topic));
            if(len > iInsertLen)
                len = iInsertLen;
            memcpy(pcInsert, cfg->topic, len);
            return len;
        }
    }

    printf("Unknown SSI tag: %d\n", tagIndex);
    return 0;
 }

void WebServer::Start()
{
    httpd_init();

    http_set_ssi_handler(HandleResponseEntry, tags, LWIP_ARRAYSIZE(tags));

}

bool WebServer::HandleRequest(fs_file *file, const char *uri, int iNumParams, char **pcParam, char **pcValue)
{
    printf("CGI executed: %s\n", uri);

    for(auto a = 0; a < iNumParams; a++)
    {
        printf("    %s = %s\n", pcParam[a], pcValue[a]);
    }

    if(!strcmp(uri, "/api/command.json"))
    {
        return DispatchCommand(iNumParams, pcParam, pcValue);
    }
    else if(!strcmp(uri, "/api/configure.json"))
    {
        std::map<std::string, std::string> params;
        for(auto a = 0; a < iNumParams; a++)
        {
            params[pcParam[a]] = pcValue[a];
        }

        if(params["mode"] == "wifi")
        {
            if(!params.count("ssid") ||
                !params.count("password"))
                return false;

            WifiConfig cfg;
            memset(&cfg, 0, sizeof(cfg));
            strlcpy(cfg.ssid, params["ssid"].c_str(), sizeof(cfg.ssid));
            auto &password = params["password"];
            if(password == "********")
            {
                auto oldCConfig = _config->GetWifiConfig();
                strlcpy(cfg.password, oldCConfig->password, sizeof(cfg.password));
            }

            puts("Saving config\n");
            _config->SaveWifiConfig(&cfg);
            puts("Restarting service\n");
            _service.StopService();
            return true;
        }
        else if(params["mode"] == "mqtt")
        {
            puts("Setting MQTT config\n");
            if(!params.count("host") ||
                !params.count("port") ||
                !params.count("username") ||
                !params.count("password") ||
                !params.count("topic"))
            {
                puts("Missing arguments\n");
                return false;
            }

            MqttConfig cfg;
            memset( &cfg, 0, sizeof(cfg));
            strlcpy(cfg.brokerAddress, params["host"].c_str(), sizeof(cfg.brokerAddress));
            sscanf(params["port"].c_str(), "%hu", &cfg.port);
            strlcpy(cfg.username, params["username"].c_str(), sizeof(cfg.username));
            auto &password = params["password"];
            if(password == "********")
            {
                auto oldCConfig = _config->GetMqttConfig();
                strlcpy(cfg.password, oldCConfig->password, sizeof(cfg.password));
            }
            else
                strlcpy(cfg.password, password.c_str(), sizeof(cfg.password));
            strlcpy(cfg.topic, params["topic"].c_str(), sizeof(cfg.topic));


            puts("Saving config\n");
            _config->SaveMqttConfig(&cfg);
            puts("Restarting service\n");
            _service.StopService();
            return true;
        }
        else if(params["mode"] == "firmware")
        {
            puts("Requesting firmware reboot\n");
            _service.StopService(true);
            return true;
        }
    }

    return false;
}

void WebServer::SubscribeCommand(uint32_t objectId, std::string command, CgiSubscribeFunc &&callback)
{
    _subscriptions.insert({ std::make_pair(objectId, command), callback});

}

void WebServer::UnsubscribeCommand(uint32_t objectId, std::string command)
{
    _subscriptions.erase(std::make_pair(objectId, command));
}


bool WebServer::DispatchCommand(int iNumParams, char **pcParam, char **pcValue)
{
    if(iNumParams < 3)
    {
        puts("Bad command received - not enough arguments\n");
        return false;
    }
    const char *idstr = nullptr;
    const char *command = nullptr;
    const char *payload = nullptr;

    for(auto a = 0; a < iNumParams; a++)
    {
        if(!strcmp(pcParam[a], "command"))
            command = pcValue[a];
        else if(!strcmp(pcParam[a], "id"))
            idstr = pcValue[a];
        else if(!strcmp(pcParam[a], "payload"))
            payload = pcValue[a];
    }

    if(!idstr || !command || !payload)
    {
        puts("Bad command received - missing arguments\n");
        return false;
    }

    uint32_t id;
    sscanf(idstr, "%d", &id);

    auto sub = _subscriptions.find(std::make_pair(id, command));
    if(sub == _subscriptions.end())
    {
        printf("No subscriber for (%s-%s)=%s\n", idstr, command, payload);
        return false;
    }

    sub->second((const uint8_t *)payload, strlen(payload));
    return true;
}
