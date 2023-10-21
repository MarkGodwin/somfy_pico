
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/pio.h"
#include "pico/cyw43_arch.h"
#include "pico/flash.h"
#include "wifiScanner.h"


WifiScanner::WifiScanner()
{
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
    while(cyw43_wifi_scan_active(&cyw43_state))
    {
        printf("Waiting for scan to finish...");
        sleep_ms(1000);
    }
}

void WifiScanner::CollectResults()
{
    if(cyw43_wifi_scan_active(&cyw43_state))
        return;

    if(_activeResults.empty())
    {
        puts("No new results found\n");
        return;
    }

    printf("%d new SSID results found", _activeResults.size());
    std::swap(_ssids, _activeResults);
    _activeResults.clear();
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
        
    printf("ssid: %-32s rssi: %4d chan: %3d mac: %02x:%02x:%02x:%02x:%02x:%02x sec: %u\n",
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
