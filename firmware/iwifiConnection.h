// Copyright (c) 2023 Mark Godwin.
// SPDX-License-Identifier: MIT

#pragma once

class IWifiConnection
{
    public:
        virtual bool IsConnected() = 0;
        virtual bool IsAccessPointMode() = 0;
};
