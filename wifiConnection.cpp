
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "wifiConnection.h"
#include "deviceConfig.h"

WifiConnection::WifiConnection(std::shared_ptr<DeviceConfig> config, bool apMode)
: _apMode(apMode),
  _config(std::move(config))
{
    // Start in STA mode so we can do an initial WiFi scan before switching to AP mode
    cyw43_arch_enable_sta_mode();
}

void WifiConnection::Start()
{

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
        auto cfg = _config->GetWifiConfig();

        printf("Connecting to WiFi %s (%s)\n", cfg->ssid, cfg->password);
        auto result = cyw43_arch_wifi_connect_blocking(cfg->ssid, cfg->password, CYW43_AUTH_WPA2_AES_PSK);
        if(result)
        {
            printf("Failed to connect: %d\n", result);
            printf("Reconnecting later...\n");
        }
        else
            printf("Connected connect: %d\n", result);

        _wifiWatchdog = std::make_unique<ScheduledTimer>([this]() { return WifiWatchdog(); }, 1000);
    }

}

bool WifiConnection::IsConnected()
{
    return _apMode || cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA) == CYW43_LINK_UP;
}


uint32_t WifiConnection::WifiWatchdog()
{
    auto state = cyw43_wifi_link_status(&cyw43_state, CYW43_ITF_STA);

    // Indicate the state with the LED
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, state > 0);
    if(state <= 0)
    {
        printf("Wifi not connected (%d)\n", state);

        auto cfg = _config->GetWifiConfig();
        printf("Reconnecting to WiFi %s (%s)\n", cfg->ssid, cfg->password);
        auto result = cyw43_arch_wifi_connect_async(cfg->ssid, cfg->password, CYW43_AUTH_WPA2_AES_PSK);
        return 1000;
    }
    else
    {
        // Restart timer
        printf("Woof (%d)\n", state);
       
        return 60000;
    }
}

