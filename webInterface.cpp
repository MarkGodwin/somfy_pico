
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "pico/flash.h"
#include "lwip/apps/httpd.h"

#include "webInterface.h"
#include "deviceConfig.h"

// HTTPD callbacks aren't conducive to multiple instances
static WebInterface *_globalInstance;

WebInterface *WebInterface::GetInstance(DeviceConfig &config)
{
    if(!_globalInstance)
    {
        _globalInstance = new WebInterface(config);
    }
    return _globalInstance;
}


extern "C" {
    // Will be called before processing any URL with a query string
    void httpd_cgi_handler(struct fs_file *file, const char* uri, int iNumParams,
                                char **pcParam, char **pcValue, void *connectionState)
    {
        WebInterface *pThis = (WebInterface *)connectionState;
        pThis->HandleRequest(file, uri, iNumParams, pcParam, pcValue);
    }

    void *fs_state_init(struct fs_file *file, const char *name)
    {
        // Hack
        return _globalInstance;
    }
    /** This user-defined function is called when a file is closed. */
    void fs_state_free(struct fs_file *file, void *state)
    {
        // Do nothing, as we didn't allocate it
    }
}


 uint16_t WebInterface::HandleResponseEntry(int tagIndex, char *pcInsert, int iInsertLen, uint16_t tagPart, uint16_t *nextPart, void *connectionState)
 {
    puts("HandleResponse\n");
    auto pThis = (WebInterface *)connectionState;
    return pThis->HandleResponse(tagIndex, pcInsert, iInsertLen, tagPart, nextPart);
 }

const char *tags[] = {
    "ssid",
    "ssidList",
    "result"
};

 uint16_t WebInterface::HandleResponse(int tagIndex, char *pcInsert, int iInsertLen, uint16_t tagPart, uint16_t *nextPart)
 {
    if(tagIndex == 0)
    {
        printf("SSI: ssid\n");
        auto cfg = _config.GetWifiConfig();
        *pcInsert++ = '\"';
        auto ssidLen = strlen(cfg->ssid);
        auto safeLen = ssidLen < iInsertLen ? ssidLen : iInsertLen;
        strncpy(pcInsert, cfg->ssid, safeLen);
        pcInsert += safeLen;
        *pcInsert++ = '\"';
        return ssidLen + 2;
    }
    if(tagIndex == 1)
    {
        printf("SSI: ssids Index: %d\n", tagPart);
        if(tagPart == 0)
        {
            _wifiScanner.CollectResults();
        }

        auto &ssids = _wifiScanner.GetSsids();
        char *writeOff = pcInsert;
        if(tagPart >= ssids.size())
            return 0;

        *writeOff++ = '\"';
        auto len = ssids[tagPart].size();
        memcpy(writeOff, ssids[tagPart].data(), len);
        writeOff += len;
        *writeOff++ = '\"';
        if(tagPart + 1 != ssids.size())
        {
            *writeOff++ = ',';
            *nextPart = tagPart + 1;
        }

        return writeOff - pcInsert;

    }
    if(tagIndex == 2)
    {
        memcpy(pcInsert, "true", 4);
        return 4;
    }

    printf("Unknown SSI tag: %d\n", tagIndex);
    return 0;
 }

void WebInterface::Start()
{
    auto cfg = _config.GetWifiConfig();
    _apMode = strlen(cfg->ssid) == 0;

    cyw43_arch_enable_sta_mode();

    _wifiScanner.TriggerScan();
    _wifiScanner.WaitForScan();
    _wifiScanner.CollectResults();

    // In AP mode we will only scan WiFi once at startup, and then switch into AP Mode.
    if(_apMode)
    {
        puts("Disabling scan mode");
        cyw43_arch_disable_sta_mode();

        const char *ap_name = "picow_test";
        const char *password = "password";

        puts("Enabling ap mode");
        cyw43_arch_enable_ap_mode(ap_name, password, CYW43_AUTH_WPA2_AES_PSK);


        ip4_addr_t host;
        ip4_addr_t mask;
        IP4_ADDR(ip_2_ip4(&host), 192, 168, 4, 1);
        IP4_ADDR(ip_2_ip4(&mask), 255, 255, 255, 0);

        // Start the dhcp server
        dhcp_server_init(&_dhcp_server, &host, &mask);
        // Start the dns server
        dns_server_init(&_dns_server, &host);

    }
    else
    {
        printf("Connecting to WiFi %s\n", cfg->ssid, cfg->password);
        auto result = cyw43_arch_wifi_connect_blocking(cfg->ssid, cfg->password, CYW43_AUTH_WPA2_AES_PSK);
        if(result)
        {
            printf("Failed to connect\n: %d", result);
        }
        else
            printf("Connected connect\n: %d", result);

        _wifiWatchdogWorker.next = nullptr;
        _wifiWatchdogWorker.do_work = WifiWatchdogEntry;
        _wifiWatchdogWorker.user_data = this;

        async_context_add_at_time_worker_in_ms(cyw43_arch_async_context(), &_wifiWatchdogWorker, 10000);
    }

    httpd_init();

    http_set_ssi_handler(HandleResponseEntry, tags, LWIP_ARRAYSIZE(tags));

}

void WebInterface::HandleRequest(fs_file *file, const char *uri, int iNumParams, char **pcParam, char **pcValue)
{
    printf("CGI executed: %s\n", uri);

    for(auto a = 0; a < iNumParams; a++)
    {
        printf("    %s = %s\n", pcParam[a], pcValue[a]);
    }

    if(!strcmp(uri, "/api/configure.json"))
    {
        const char *mode = nullptr;
        const char *ssid = nullptr;
        const char *password = nullptr;
        const char *host = nullptr;
        const char *port = nullptr;
        const char *username = nullptr;

        for(auto a = 0; a < iNumParams; a++)
        {
            if(!strcmp(pcParam[a], "mode"))
                mode = pcValue[a];
            else if(!strcmp(pcParam[a], "ssid"))
                ssid = pcValue[a];
            else if(!strcmp(pcParam[a], "password"))
                password = pcValue[a];
            else if(!strcmp(pcParam[a], "host"))
                host = pcValue[a];
            else if(!strcmp(pcParam[a], "port"))
                port = pcValue[a];
            else if(!strcmp(pcParam[a], "username"))
                username = pcValue[a];
        }

        if(mode != nullptr)
        {
            if(!strcmp(mode, "wifi") && ssid && password)
            {
                WifiConfig cfg;
                memset(&cfg, 0, sizeof(cfg));
                strcpy(cfg.ssid, ssid);
                strcpy(cfg.password, password);
                _config.SaveWifiConfig(&cfg);
            }
        }
    }
}

 void WebInterface::WifiWatchdogEntry(async_context_t *context, async_at_time_worker_t *worker) {

    auto pThis = (WebInterface *)worker->user_data;
    pThis->WifiWatchdog(context);
}

void WebInterface::WifiWatchdog(async_context_t *context)
{
    printf("Woof\n");
    auto state = cyw43_wifi_link_status(&cyw43_state, CYW43_ITF_STA);
    // Indicate the state with the LED
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, state == CYW43_LINK_JOIN);

    // Restart timer
    async_context_add_at_time_worker_in_ms(context, &_wifiWatchdogWorker, 60000);
}

