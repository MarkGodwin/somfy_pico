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
#include "remotes.h"
#include "blinds.h"
#include "wifiConnection.h"
#include "wifiScanner.h"
#include "webServer.h"
#include "mqttClient.h"
#include "blockStorage.h"
#include "configService.h"
#include "deviceConfig.h"
#include "serviceStatus.h"
#include "serviceControl.h"
#include "statusLed.h"
#include "commandQueue.h"

// SPI Defines
// We are going to use SPI 0, and allocate it to the following GPIO pins
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define SPI_PORT spi_default
#define PIN_MISO 4
#define PIN_SCK  2
#define PIN_MOSI 3
#define PIN_CS_RADIO   5

// Hard buttons
#define PIN_RESET 0
#define PIN_WIFI 1

// Status LED
#define PIN_LED_R 11
#define PIN_LED_G 12
#define PIN_LED_B 13

// How much flash memory to use to store blind/remote/wifi config
#define STORAGE_SECTORS 32
#define STORAGE_BLOCK_SIZE 252

// Hardware reset
#define PIN_RESET_RADIO 15

const WifiConfig *checkConfig(
    std::shared_ptr<DeviceConfig> config);

void runServiceMode(
    std::shared_ptr<WebServer> webServer,
    std::shared_ptr<DeviceConfig> config,
    std::shared_ptr<WifiConnection> wifiConnection,
    std::shared_ptr<RadioCommandQueue> commandQueue,
    std::shared_ptr<ServiceControl> service);

void runSetupMode(
    std::shared_ptr<WebServer> webServer,
    std::shared_ptr<ServiceControl> service);

bool checkButtonHeld(int inputPin, StatusLed &led);

StatusLed redLed(PIN_LED_R);
StatusLed greenLed(PIN_LED_G);
StatusLed blueLed(PIN_LED_B);


int main()
{
    stdio_init_all();

    //gpio_init(PIN_LED_R);
    //gpio_set_dir(PIN_LED_R, GPIO_OUT);
    //gpio_init(PIN_LED_G);
    //gpio_set_dir(PIN_LED_G, GPIO_OUT);
    //gpio_init(PIN_LED_B);
    //gpio_set_dir(PIN_LED_B, GPIO_OUT);

    gpio_init(PIN_RESET);
    gpio_set_dir(PIN_RESET, GPIO_IN);
    gpio_set_pulls(PIN_RESET, true, false);
    gpio_init(PIN_WIFI);
    gpio_set_dir(PIN_WIFI, GPIO_IN);
    gpio_set_pulls(PIN_WIFI, true, false);

    // SPI initialisation. Set SPI speed to 0.5MHz. Absolute max supported is 10Mhz
    spi_init(SPI_PORT, 500*1000);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);


    for(auto a = 0; a < 512; a+=2)
    {
        redLed.SetLevel(a);
        sleep_ms(10);
    }
    for(auto a = 0; a < 512; a+=2)
    {
        greenLed.SetLevel(a);
        sleep_ms(10);
    }
    for(auto a = 0; a < 512; a+=2)
    {
        blueLed.SetLevel(a);
        sleep_ms(10);
    }
    redLed.SetLevel(0);
    greenLed.SetLevel(0);
    blueLed.SetLevel(0);

   
    if (cyw43_arch_init()) {
        printf("Wi-Fi init failed");
        redLed.SetLevel(1024);
        greenLed.SetLevel(0);
        blueLed.SetLevel(0);
        return -1;
    }

    puts("Starting the Radio\n");
    auto radio = std::make_shared<RFM69Radio>(SPI_PORT, PIN_CS_RADIO, PIN_RESET_RADIO);

    // We'll run radio commands from the core 1 thread
    auto commandQueue = std::make_shared<RadioCommandQueue>(radio, &blueLed);
    puts("Starting worker thread...\n");
    commandQueue->Start();

    // Store flash settings at the very top of Flash memory
    // ALL STORED DATA WILL BE LOST IF THESE ARE CHANGED
    const uint32_t StorageSize = STORAGE_SECTORS * FLASH_SECTOR_SIZE;

    auto config = std::make_shared<DeviceConfig>(StorageSize, STORAGE_BLOCK_SIZE);
    auto wifiConfig = checkConfig(config);
    if(wifiConfig == nullptr)
    {
        redLed.SetLevel(1024);
        greenLed.SetLevel(0);
        blueLed.SetLevel(1024);
        return -1;
    }

    auto apMode = !wifiConfig->ssid[0];

    puts("Starting the web interface...\n");

    auto service = std::make_shared<ServiceControl>();

    if(checkButtonHeld(PIN_WIFI, greenLed))
    {
        puts("Starting in WIFI Setup mode, as button WIFI button is pressed");
        apMode = true;
    }

    auto wifiConnection = std::make_shared<WifiConnection>(config, apMode);

    // Do an initial WiFi scan before entering AP mode. We don't seem to be able
    // to scan once in AP mode.
    auto wifiScanner = std::make_shared<WifiScanner>(apMode);
    wifiScanner->WaitForScan();

    // Now connect to WiFi or Enable AP mode
    wifiConnection->Start();

    auto webServer = std::make_shared<WebServer>(config, wifiConnection);
    webServer->Start();
    auto configService = std::make_shared<ConfigService>(config, webServer, wifiScanner, service);

    if(apMode)
    {
        runSetupMode(webServer, service);
    }
    else
    {
        runServiceMode(webServer, config, wifiConnection, commandQueue, service);
    }

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

}

bool checkButtonHeld(int inputPin, StatusLed &led)
{
    int a = 0;
    for(a = 0; !gpio_get(inputPin) && a < 100; a++)
    {
        led.SetLevel((a * 128) & 1023);
        sleep_ms(50);
    }
    led.SetLevel(0);
    return a == 100;
}

const WifiConfig *checkConfig(
    std::shared_ptr<DeviceConfig> config)
{
    auto needReset = config->GetWifiConfig() == nullptr;
    if(needReset)
        puts("WiFi Config not found.\n");
    else
    {
        needReset = checkButtonHeld(PIN_RESET, redLed);
    }

    if(needReset)
    {
        puts("Resetting all settings!");
        config->HardReset();
    }

    puts("Checking Device config...\n");

    auto wifiConfig = config->GetWifiConfig();
    if(wifiConfig == nullptr ||
        config->GetMqttConfig() == nullptr)
    {
        puts("Settings could not be read back. Error!\n");
        return nullptr;
    }
    return wifiConfig;
}

void runServiceMode(
    std::shared_ptr<WebServer> webServer,
    std::shared_ptr<DeviceConfig> config,
    std::shared_ptr<WifiConnection> wifiConnection,
    std::shared_ptr<RadioCommandQueue> commandQueue,
    std::shared_ptr<ServiceControl> service)
{

    auto mqttClient = std::make_shared<MqttClient>(config, wifiConnection, "pico_somfy/status", "online", "offline");

    mqttClient->Start();
    // Subscribe by wildcard to reduce overhead
    mqttClient->SubscribeTopic("pico_somfy/blinds/+/cmd");
    mqttClient->SubscribeTopic("pico_somfy/blinds/+/pos");
    mqttClient->SubscribeTopic("pico_somfy/remotes/+/cmd");

    auto blinds = std::make_shared<Blinds>(config, mqttClient, webServer);
    auto remotes = std::make_shared<SomfyRemotes>(config, blinds, webServer, commandQueue, mqttClient);
    blinds->Initialize(remotes);

    auto onOff = 0x04;
    
    ServiceStatus statusApi(webServer, mqttClient, false);

    ScheduledTimer republishTimer([&blinds, &remotes] () {
        // Try to republish discovery information for one device
        // It should be safe to access the collections from this callback.
        if(remotes->TryRepublish()||
            blinds->TryRepublish())
            // Do another later, if there is more work to be done
            // We can't publish everything at once, as lwip needs a chance to run to clear the queues
            return 100;

        // Everything that needed to be published has been
        return 0;
    }, 0);

    

    auto mqttConnected = false;

    puts("Entering main loop...\n");
    bool resetPushed = false;
    while(!service->IsStopRequested()) {

        // We're using background IRQ callbacks from LWIP, so we can just sleep peacfully...
        sleep_ms(500);

        // Lazy man's notification of MQTT reconnection outside of an IRQ callback
        if(!mqttConnected)
        {
            if(mqttClient->IsConnected())
            {
                mqttConnected = true;
                republishTimer.ResetTimer(250);
            }
        }
        else if(!mqttClient->IsConnected())
        {
            republishTimer.ResetTimer(0);
            mqttConnected = false;
        }

        // Show Mark we aren't dead
        redLed.SetLevel( wifiConnection->IsConnected() ?
            ((onOff & 0x01) ? 1024 : 512) : 0);
        greenLed.SetLevel( mqttConnected ?
            ((onOff & 0x04) ? 512 : 256) : 0);
        if(onOff == 0)
            onOff = 0x04;
        else
            onOff >>= 1;

        if(!gpio_get(PIN_RESET))
        {
            if(resetPushed)
            {
                puts("Reset pushed!");
                service->StopService(false);
            }
            resetPushed = true;
        }
        else
        {
            resetPushed = false;
        }
        if(!gpio_get(PIN_WIFI))
        {
            puts("WIFI pushed");
        }

    }

    republishTimer.ResetTimer(0);

    puts("Waiting for the command queue to clear...\n");
    commandQueue->Shutdown();


    puts("Saving any unsaved state.\n");
    remotes->SaveRemoteState();
    blinds->SaveBlindState(true);

}

void runSetupMode(
    std::shared_ptr<WebServer> webServer,
    std::shared_ptr<ServiceControl> service)
{
    ServiceStatus statusApi(webServer, nullptr, true);

    puts("Entering setup loop...\n");
    auto onOff = false;
    while(!service->IsStopRequested()) {

        // We're using background IRQ callbacks from LWIP, so we can just sleep peacfully...
        sleep_ms(1000);

        // Show Mark we aren't dead
        greenLed.SetLevel(onOff ? 1024 : 512);
        onOff = !onOff;

        if(!gpio_get(PIN_RESET))
        {
            printf("Reset pushed\n");
            service->StopService();
        }
        if(!gpio_get(PIN_WIFI))
            printf("WIFI pushed\n");
    }

}