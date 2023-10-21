#pragma once

#include "wifiScanner.h"
#include "dhcpserver.h"
#include "dnsserver.h"
#include <memory>
#include <functional>
#include <map>

class DeviceConfig;
class ServiceControl;
class IWifiConnection;
class WifiScanner;


typedef std::function<void(const uint8_t *, uint32_t)> CgiSubscribeFunc;


class WebServer
{
    public:
        WebServer(std::shared_ptr<DeviceConfig> config, ServiceControl &service, std::shared_ptr<IWifiConnection> wifiConnection, std::shared_ptr<WifiScanner> wifiScanner, bool apMode);

        void Start();

        bool HandleRequest(struct fs_file *file, const char* uri, int iNumParams, char **pcParam, char **pcValue);

        void SubscribeCommand(uint32_t objectId, std::string command, CgiSubscribeFunc &&callback);
        void UnsubscribeCommand(uint32_t objectId, std::string command);

    private:

        static uint16_t HandleResponseEntry(int tagIndex, char *pcInsert, int iInsertLen, uint16_t tagPart, uint16_t *nextPart, void *connectionState);
        uint16_t HandleResponse(int tagIndex, char *pcInsert, int iInsertLen, uint16_t tagPart, uint16_t *nextPart, bool cgiResult);

        bool DispatchCommand(int iNumParams, char **pcParam, char **pcValue);

        bool _apMode;
        std::shared_ptr<DeviceConfig> _config;
        ServiceControl &_service;
        std::shared_ptr<IWifiConnection> _wifiConnection;
        std::shared_ptr<WifiScanner> _wifiScanner;

        std::map<std::pair<uint32_t, std::string>, CgiSubscribeFunc> _subscriptions;

};

class CgiSubscription
{
    public:
        CgiSubscription(std::shared_ptr<WebServer> webInterface, uint32_t objectId, std::string command, CgiSubscribeFunc &&callback)
        :   _webInterface(webInterface),
             _objectId(objectId),
             _command(std::move(_command))
        {
            _webInterface->SubscribeCommand(_objectId, _command, std::forward<CgiSubscribeFunc>(callback));
        }

        ~CgiSubscription()
        {
            _webInterface->UnsubscribeCommand(_objectId, _command);
        }
    private:
        std::shared_ptr<WebServer> _webInterface;
        uint32_t _objectId;
        std::string _command;
};
