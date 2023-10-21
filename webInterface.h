#pragma once

#include "wifiScanner.h"
#include "dhcpserver.h"
#include "dnsserver.h"


class DeviceConfig;
class ServiceControl;
class IWifiConnection;
class WifiScanner;

class WebInterface
{
    public:
        WebInterface(DeviceConfig &config, ServiceControl &service, IWifiConnection &wifiConnection, WifiScanner &wifiScanner, bool apMode);

        void Start();

        bool HandleRequest(struct fs_file *file, const char* uri, int iNumParams, char **pcParam, char **pcValue);

    private:

        static uint16_t HandleResponseEntry(int tagIndex, char *pcInsert, int iInsertLen, uint16_t tagPart, uint16_t *nextPart, void *connectionState);
        uint16_t HandleResponse(int tagIndex, char *pcInsert, int iInsertLen, uint16_t tagPart, uint16_t *nextPart, bool cgiResult);

        bool _apMode;
        DeviceConfig &_config;
        ServiceControl &_service;
        IWifiConnection &_wifiConnection;
        WifiScanner &_wifiScanner;


};
