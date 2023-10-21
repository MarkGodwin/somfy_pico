#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/pio.h"
#include "hardware/watchdog.h"
#include "pico/cyw43_arch.h"
#include "pico/flash.h"
#include "pico/bootrom.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"

#include "radio.h"
#include "remote.h"
#include "remotes.h"
#include "blinds.h"
#include "wifiConnection.h"
#include "wifiScanner.h"
#include "webInterface.h"
#include "mqttClient.h"
#include "blockStorage.h"
#include "configService.h"
#include "deviceConfig.h"
#include "serviceControl.h"
#include "commandQueue.h"

// SPI Defines
// We are going to use SPI 0, and allocate it to the following GPIO pins
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define SPI_PORT spi_default
#define PIN_MISO 4
#define PIN_SCK  6
#define PIN_MOSI 7

// Status LED
#define PIN_LED 13

#define PIN_CS_RADIO   5
// Hardware reset
#define PIN_RESET_RADIO 15


int main()
{
    stdio_init_all();

    gpio_init(PIN_LED);
    gpio_set_dir(PIN_LED, GPIO_OUT);

    // SPI initialisation. Set SPI speed to 0.5MHz. Absolute max supported is 10Mhz
    spi_init(SPI_PORT, 500*1000);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);

    sleep_ms(5000);
    
    if (cyw43_arch_init()) {
        printf("Wi-Fi init failed");
        return -1;
    }


    // Store flash settings at the very top of Flash memory
    // ALL STORED DATA WILL BE LOST IF THESE ARE CHANGED
    const uint32_t StorageSize = 8 * FLASH_SECTOR_SIZE;

    auto config = std::make_shared<DeviceConfig>(StorageSize, 252);
    if(config->GetWifiConfig() == nullptr)
    {
        puts("WiFi Config not found. Resetting all settings!\n");
        config->HardReset();
    }

    puts("Checking Device config...\n");

    auto wifiConfig = config->GetWifiConfig();
    if(wifiConfig == nullptr ||
        config->GetMqttConfig() == nullptr)
    {
        puts("Settings could not be read back. Error!\n");
        return -1;
    }


    puts("Starting the web interface...\n");

    auto service = std::make_shared<ServiceControl>();

    // TODO: Read an input pin for forcing AP mode
    auto apMode = !wifiConfig->ssid[0];

    auto wifiConnection = std::make_shared<WifiConnection>(config, apMode);

    // Do an initial WiFi scan before entering AP mode
    auto wifiScanner = std::make_shared<WifiScanner>(apMode);
    wifiScanner->WaitForScan();

    // Now connect to WiFi or Enable AP mode
    wifiConnection->Start();

    auto webServer = std::make_shared<WebServer>(config, wifiConnection);
    webServer->Start();

    auto configService = std::make_shared<ConfigService>(config, webServer, wifiScanner, service);

    auto mqttClient = std::make_shared<MqttClient>(config, wifiConnection, "pico_somfy/status", "online", "offline");
    if(!apMode)
    {
        mqttClient->Start();

        mqttClient->Subscribe("pico_somfy/test", [](const uint8_t *msg, uint32_t len) {
            printf("Message recieved: %.*s\n", len, msg);
        });
    }

    auto radio = std::make_shared<RFM69Radio>(SPI_PORT, PIN_CS_RADIO, PIN_RESET_RADIO);
    auto commandQueue = std::make_shared<RadioCommandQueue>(radio);

    auto blinds = std::make_shared<Blinds>(config, mqttClient, webServer);
    auto remotes = std::make_shared<SomfyRemotes>(config, blinds, commandQueue);
    blinds->Initialize(remotes);

    auto onOff = false;

    
    puts("Starting worker thread...\n");
    commandQueue->Start();

    puts("Entering main loop...\n");
    while(!service->IsStopRequested()) {

        // We're using background IRQ callbacks from LWIP, so we can just sleep peacfully...
        sleep_ms(1000);

        gpio_put(PIN_LED, onOff);
        onOff = !onOff;
    }

    puts("Waiting for the command queue to clear...\n");
    commandQueue->Shutdown();

    puts("Restarting now!\n");
    sleep_ms(1000);

    if(service->IsFirmwareUpdateRequested())
    {
        reset_usb_boot(0,0);
    }
    else
    {
        watchdog_reboot(0,0,500);
    }
    sleep_ms(5000);
    puts("ERM!!! Why no reboot?\n");

    // dns_server_deinit(&dns_server);
    // dhcp_server_deinit(&dhcp_server);
    // cyw43_arch_deinit();
    // return 0;

/*    RFM69Radio radio(SPI_PORT, PIN_CS_RADIO, PIN_RESET_RADIO);


    sleep_ms(6000);
    puts("Starting...\n");

    radio.Reset();
    puts("Reset complete!\n");
    radio.Initialize();

    radio.SetSymbolWidth(635);
    radio.SetFrequency(433.44);

    auto fq = radio.GetFrequency();
    printf("Radio frequency: %.3f\n", fq);
    auto br = radio.GetBitRate();
    printf("BitRate: %dbps\n", br);
  
    auto ver = radio.GetVersion();
    printf("Radio version: 0x%02x\n", ver);

    SomfyRemote remote(&radio, 0x27962a, 2612);


    uint16_t loopCounter = 0;
    while (true) {
        loopCounter++;
        //cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        gpio_put(PIN_LED, 0);
        sleep_ms(250);
        //cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        sleep_ms(250);
        //cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        gpio_put(PIN_LED, 1);
        sleep_ms(250);
        //cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        sleep_ms(250);

        if((loopCounter & 0x1F) == 0x0F)
        {
            remote.PressButtons(SomfyButton::Up, 4);
            puts("Going Up...!\n");
        }
        if((loopCounter & 0x1F) == 0x1F)
        {
            remote.PressButtons(SomfyButton::Down, 4);
            puts("Going Down...!\n");

            return 0;
        }

    }
    puts("Hello, world!");

    return 0;*/
}
