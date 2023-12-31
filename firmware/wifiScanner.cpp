// Copyright (c) 2023 Mark Godwin.
// SPDX-License-Identifier: MIT

#include <stdio.h>
#include "picoSomfy.h"
#include "hardware/spi.h"
#include "hardware/pio.h"
#include "pico/cyw43_arch.h"
#include "pico/flash.h"
#include "wifiScanner.h"


WifiScanner::WifiScanner(bool oneTimeScan)
:   _oneTimeScan(oneTimeScan)
{
    TriggerScan();
}

void WifiScanner::TriggerScan()
{
    if(cyw43_wifi_scan_active(&cyw43_state))
        return;

    _activeResults.clear();

    cyw43_wifi_scan_options_t scan_options = {0};
    int err = cyw43_wifi_scan(&cyw43_state, &scan_options, this, ScanResultEntry);

}

void WifiScanner::WaitForScan()
{
    DBG_PUT("Waiting for Wifi scan to finish...");
    while(cyw43_wifi_scan_active(&cyw43_state))
    {
#if PICO_CYW43_ARCH_POLL
        auto wakeTime = make_timeout_time_ms(1000);
        cyw43_arch_wait_for_work_until(wakeTime);
        cyw43_arch_poll();
#else
        sleep_ms(1000);
#endif
    }
    DBG_PUT("Wifi scan complete.");
}

void WifiScanner::CollectResults()
{
    if(cyw43_wifi_scan_active(&cyw43_state))
        return;

    if(_activeResults.empty())
    {
        DBG_PUT("No new results found\n");
        return;
    }

    DBG_PRINT("%d new SSID results found", _activeResults.size());
    std::swap(_ssids, _activeResults);
    _activeResults.clear();

    // Trigger another scan so we have new results next time the caller asks
    if(!_oneTimeScan)
        TriggerScan();
}


int WifiScanner::ScanResultEntry(void *env, const cyw43_ev_scan_result_t *result)
{
    auto pThis = (WifiScanner *)env;
    return pThis->ScanResult(result);
}

int WifiScanner::ScanResult(const cyw43_ev_scan_result_t *result)
{
    if (!result)
        return 0;
        
    DBG_PRINT("ssid: %-32s rssi: %4d chan: %3d mac: %02x:%02x:%02x:%02x:%02x:%02x sec: %u\n",
        result->ssid, result->rssi, result->channel,
        result->bssid[0], result->bssid[1], result->bssid[2], result->bssid[3], result->bssid[4], result->bssid[5],
        result->auth_mode);


    // Avoid duplicates
    for(auto it = _activeResults.begin(); it != _activeResults.end(); it++)
    {
        if(!it->compare((const char *)result->ssid))
            return 0;
    }

    _activeResults.push_back((const char *)result->ssid);
    return 0;
}
