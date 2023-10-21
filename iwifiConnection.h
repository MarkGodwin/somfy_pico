#pragma once

class IWifiConnection
{
    public:
        virtual bool IsConnected() = 0;
        virtual bool IsAccessPointMode() = 0;
};
