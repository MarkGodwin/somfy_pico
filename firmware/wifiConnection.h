#pragma once

#include "dhcpserver.h"
#include "dnsserver.h"
#include "iwifiConnection.h"
#include "scheduler.h"
#include <memory>

class DeviceConfig;

/// @brief Trivial wrapper around wifi API to handle connection status
class WifiConnection : public IWifiConnection
{
    public:
        WifiConnection(std::shared_ptr<DeviceConfig> config, bool apMode);
        void Start();

        virtual bool IsConnected();
        virtual bool IsAccessPointMode() { return _apMode; }

    private:    
        uint32_t WifiWatchdog();

        std::unique_ptr<ScheduledTimer> _wifiWatchdog;
        std::shared_ptr<DeviceConfig> _config;
        bool _apMode;
        // For AP mode        
        dhcp_server_t _dhcp_server;
        dns_server_t _dns_server;

};
