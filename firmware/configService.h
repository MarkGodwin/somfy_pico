#pragma once

#include "deviceConfig.h"
#include "webServer.h"
#include <memory>
#include <list>

class ServiceControl;
class WifiScanner;

/// @brief Handles configuration via the web interface
class ConfigService
{
    public:
        ConfigService(
            std::shared_ptr<DeviceConfig> config,
            std::shared_ptr<WebServer> webServer,
            std::shared_ptr<WifiScanner> wifiScanner,
            std::shared_ptr<ServiceControl> serviceControl);

    private:
        bool OnConfigure(const CgiParams &params);

        int16_t HandleWifiConfigResponse(uint32_t tagIndex, char *pcInsert, int iInsertLen, uint16_t tagPart, uint16_t *nextPart);
        int16_t HandleMqttConfigResponse(uint32_t tagIndex, char *pcInsert, int iInsertLen, uint16_t tagPart, uint16_t *nextPart);

        std::shared_ptr<DeviceConfig> _config;
        std::shared_ptr<WifiScanner> _wifiScanner;
        std::shared_ptr<ServiceControl> _serviceControl;

        CgiSubscription _configController;

        std::list<SsiSubscription> _ssiHandlers;
};
