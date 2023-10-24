// Copyright (c) 2023 Mark Godwin.
// SPDX-License-Identifier: MIT

#pragma once

#include "dhcpserver.h"
#include "dnsserver.h"
#include "iwifiConnection.h"
#include "scheduler.h"
#include <memory>

class DeviceConfig;
class StatusLed;

/// @brief Trivial wrapper around wifi API to handle connection status
class WifiConnection : public IWifiConnection
{
    public:
        WifiConnection(std::shared_ptr<DeviceConfig> config, bool apMode, StatusLed *statusLed);
        void Start();

        virtual bool IsConnected();
        virtual bool IsAccessPointMode() { return _apMode; }

    private:    
        uint32_t WifiWatchdog();

        std::unique_ptr<ScheduledTimer> _wifiWatchdog;
        std::shared_ptr<DeviceConfig> _config;
        StatusLed *_statusLed;
        bool _apMode;
        // For AP mode        
        dhcp_server_t _dhcp_server;
        dns_server_t _dns_server;

};
