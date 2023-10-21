#pragma once

#include <vector>
#include <string>

class WifiScanner
{
    public:
        WifiScanner();

        void TriggerScan();
        void WaitForScan();
        void CollectResults();

        const std::vector<std::string> &GetSsids() { return _ssids; }

    private:

        static int ScanResultEntry(void *env, const cyw43_ev_scan_result_t *result);
        int ScanResult(const cyw43_ev_scan_result_t *result);

        std::vector<std::string> _activeResults;
        std::vector<std::string> _ssids;

        bool _isScanActive;
};
