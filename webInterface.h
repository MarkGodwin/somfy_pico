#pragma once

#include "wifiScanner.h"
#include "dhcpserver.h"
#include "dnsserver.h"


class DeviceConfig;

class WebInterface
{
    public:
        static WebInterface *GetInstance(DeviceConfig &config);

        void Start();

        void HandleRequest(struct fs_file *file, const char* uri, int iNumParams, char **pcParam, char **pcValue);

    private:
        WebInterface(DeviceConfig &config)
        : _config(config) {}

        async_at_time_worker_t _wifiWatchdogWorker;
        static void WifiWatchdogEntry(async_context_t *context, async_at_time_worker_t *worker);
        void WifiWatchdog(async_context_t *context);

        static uint16_t HandleResponseEntry(int tagIndex, char *pcInsert, int iInsertLen, uint16_t tagPart, uint16_t *nextPart, void *connectionState);
        uint16_t HandleResponse(int tagIndex, char *pcInsert, int iInsertLen, uint16_t tagPart, uint16_t *nextPart);

        bool _apMode;
        DeviceConfig &_config;
        WifiScanner _wifiScanner;

        // For AP mode
        dhcp_server_t _dhcp_server;
        dns_server_t _dns_server;

};
