// Copyright (c) 2023 Mark Godwin.
// SPDX-License-Identifier: MIT

#pragma once

class ServiceControl
{
    public:
        ServiceControl() : _stopRequested( false ), _firmwareMode(false)
        {
        }

        bool IsStopRequested() const
        {
            return _stopRequested;
        }

        bool IsFirmwareUpdateRequested() const
        {
            return _firmwareMode;
        }

        void StopService(bool firmwareMode = false)
        {
            _stopRequested = true;
            _firmwareMode = firmwareMode;
        }

    private:
        volatile bool _stopRequested;
        volatile bool _firmwareMode;
};
