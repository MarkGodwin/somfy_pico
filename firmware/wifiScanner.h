// Copyright (c) 2023 Mark Godwin.
// SPDX-License-Identifier: MIT

#pragma once

#include "pico/cyw43_arch.h"
#include <vector>
#include <string>

class WifiScanner
{
    public:
        WifiScanner(bool oneTimScan);

        void WaitForScan();
        void CollectResults();

        const std::vector<std::string> &GetSsids() { return _ssids; }

    private:
        void TriggerScan();

        static int ScanResultEntry(void *env, const cyw43_ev_scan_result_t *result);
        int ScanResult(const cyw43_ev_scan_result_t *result);

        std::vector<std::string> _activeResults;
        std::vector<std::string> _ssids;
       
        bool _isScanActive;
        bool _oneTimeScan;
};
