// Copyright (c) 2023 Mark Godwin.
// SPDX-License-Identifier: MIT

#include "picoSomfy.h"

#include "configService.h"
#include "serviceControl.h"
#include "deviceConfig.h"
#include "wifiScanner.h"

#define TAGINDEX_SSID 0
#define TAGINDEX_SSIDLIST 1
#define TAGINDEX_MQTTADDR 0
#define TAGINDEX_MQTTPORT 1
#define TAGINDEX_MQTTUSER 2
#define TAGINDEX_MQTTTOPIC 3

ConfigService::ConfigService(
    std::shared_ptr<DeviceConfig> config,
    std::shared_ptr<WebServer> webServer,
    std::shared_ptr<WifiScanner> wifiScanner,
    std::shared_ptr<ServiceControl> serviceControl)
:  _config(std::move(config)),
   _wifiScanner(std::move(wifiScanner)),
   _configController(webServer, "/api/configure.json", [this](const CgiParams &params) { return OnConfigure(params); } ),
   _serviceControl(std::move(serviceControl))
{

    _ssiHandlers.push_back(SsiSubscription(webServer, "ssid", [this] (char *pcInsert, int iInsertLen, uint16_t tagPart, uint16_t *nextPart) { return HandleWifiConfigResponse(TAGINDEX_SSID, pcInsert, iInsertLen, tagPart, nextPart); }));
    _ssiHandlers.push_back(SsiSubscription(webServer, "ssidList", [this] (char *pcInsert, int iInsertLen, uint16_t tagPart, uint16_t *nextPart) { return HandleWifiConfigResponse(TAGINDEX_SSIDLIST, pcInsert, iInsertLen, tagPart, nextPart); }));

    // TODO: std::bind should make this cleaner, if only it would compile.
    _ssiHandlers.push_back(SsiSubscription(webServer, "mqttPort", [this] (char *pcInsert, int iInsertLen, uint16_t tagPart, uint16_t *nextPart) { return HandleMqttConfigResponse(TAGINDEX_MQTTPORT, pcInsert, iInsertLen, tagPart, nextPart); }));
    _ssiHandlers.push_back(SsiSubscription(webServer, "mqttUser", [this] (char *pcInsert, int iInsertLen, uint16_t tagPart, uint16_t *nextPart) { return HandleMqttConfigResponse(TAGINDEX_MQTTUSER, pcInsert, iInsertLen, tagPart, nextPart); }));
    _ssiHandlers.push_back(SsiSubscription(webServer, "mqttTopi", [this] (char *pcInsert, int iInsertLen, uint16_t tagPart, uint16_t *nextPart) { return HandleMqttConfigResponse(TAGINDEX_MQTTTOPIC, pcInsert, iInsertLen, tagPart, nextPart); }));
    _ssiHandlers.push_back(SsiSubscription(webServer, "mqttAddr", [this] (char *pcInsert, int iInsertLen, uint16_t tagPart, uint16_t *nextPart) { return HandleMqttConfigResponse(TAGINDEX_MQTTADDR, pcInsert, iInsertLen, tagPart, nextPart); }));

}

bool ConfigService::OnConfigure(const CgiParams &params)
{
    auto modeParam = params.find("mode");
    if(modeParam == params.end())
        return false;
    auto &mode = modeParam->second;

    if(mode == "wifi")
    {
        if(!params.count("ssid") ||
            !params.count("password"))
            return false;

        WifiConfig cfg;
        memset(&cfg, 0, sizeof(cfg));
        strlcpy(cfg.ssid, params.at("ssid").c_str(), sizeof(cfg.ssid));
        auto &password = params.at("password");
        if(password == "********")
        {
            auto oldCConfig = _config->GetWifiConfig();
            strlcpy(cfg.password, oldCConfig->password, sizeof(cfg.password));
        }
        else
        {
            strlcpy(cfg.password, password.c_str(), sizeof(cfg.password));
        }

        DBG_PUT("Saving config\n");
        _config->SaveWifiConfig(&cfg);
        DBG_PUT("Restarting service\n");
        _serviceControl->StopService();
        return true;
    }
    else if(mode == "mqtt")
    {
        DBG_PUT("Setting MQTT config\n");
        if(!params.count("host") ||
            !params.count("port") ||
            !params.count("username") ||
            !params.count("password") ||
            !params.count("topic"))
        {
            DBG_PUT("Missing arguments\n");
            return false;
        }

        MqttConfig cfg;
        memset( &cfg, 0, sizeof(cfg));
        strlcpy(cfg.brokerAddress, params.at("host").c_str(), sizeof(cfg.brokerAddress));
        sscanf(params.at("port").c_str(), "%hu", &cfg.port);
        strlcpy(cfg.username, params.at("username").c_str(), sizeof(cfg.username));
        auto &password = params.at("password");
        if(password == "********")
        {
            auto oldCConfig = _config->GetMqttConfig();
            strlcpy(cfg.password, oldCConfig->password, sizeof(cfg.password));
        }
        else
            strlcpy(cfg.password, password.c_str(), sizeof(cfg.password));
        strlcpy(cfg.topic, params.at("topic").c_str(), sizeof(cfg.topic));


        DBG_PUT("Saving config\n");
        _config->SaveMqttConfig(&cfg);
        DBG_PUT("Restarting service\n");
        _serviceControl->StopService();
    }
    else if(mode == "firmware")
    {
        DBG_PUT("Requesting firmware reboot\n");
        _serviceControl->StopService(true);
        return true;
    }

    return false;
}

int16_t ConfigService::HandleWifiConfigResponse(uint32_t tagIndex, char *pcInsert, int iInsertLen, uint16_t tagPart, uint16_t *nextPart)
{
    auto cfg = _config->GetWifiConfig();
    switch(tagIndex)
    {
        case TAGINDEX_SSID:
        {
            auto ssidLen = strnlen(cfg->ssid, sizeof(cfg->ssid));
            memcpy(pcInsert, cfg->ssid, ssidLen);
            return ssidLen;
        }

        // TODO: Move this to WifiScanner?
        case TAGINDEX_SSIDLIST:
        {
            if(tagPart == 0)
                _wifiScanner->CollectResults();

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
    }

    return 0;
}

int16_t ConfigService::HandleMqttConfigResponse(uint32_t tagIndex, char *pcInsert, int iInsertLen, uint16_t tagPart, uint16_t *nextPart)
{
    auto cfg = _config->GetMqttConfig();
    switch(tagIndex)
    {
        case TAGINDEX_MQTTADDR:
        {
            auto len = strnlen(cfg->brokerAddress, sizeof(cfg->brokerAddress));
            memcpy(pcInsert, cfg->brokerAddress, len);
            return len;
        }
        case TAGINDEX_MQTTPORT:
        {
            char buf[16];
            auto len = sprintf(buf, "%d", cfg->port);
            memcpy(pcInsert, buf, len);
            return len;
        }
        case TAGINDEX_MQTTUSER:
        {
            auto len = strnlen(cfg->username, sizeof(cfg->username));
            memcpy(pcInsert, cfg->username, len);
            return len;
        }
        case TAGINDEX_MQTTTOPIC:
        {
            auto len = strnlen(cfg->topic, sizeof(cfg->topic));
            if(len > iInsertLen)
                len = iInsertLen;
            memcpy(pcInsert, cfg->topic, len);
            return len;
        }
    }

    return 0;
}