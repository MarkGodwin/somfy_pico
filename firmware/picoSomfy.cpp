// Copyright (c) 2023 Mark Godwin.
// SPDX-License-Identifier: MIT
//
// Main entry point!

#include "picoSomfy.h"
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
// Pins can be changed to some extent, see the GPIO function select table in the datasheet for information on GPIO assignments
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

// How much flash memory to use to store blind/remote/wifi config. Beware changing these as it will corrupt the block storage
#define STORAGE_SECTORS 32
#define STORAGE_BLOCK_SIZE 252

// Radio hardware reset
#define PIN_RESET_RADIO 15

const WifiConfig *checkConfig(
    std::shared_ptr<DeviceConfig> config,
    StatusLed *statusLed);

void runServiceMode(
    std::shared_ptr<WebServer> webServer,
    std::shared_ptr<DeviceConfig> config,
    std::shared_ptr<WifiConnection> wifiConnection,
    std::shared_ptr<RadioCommandQueue> commandQueue,
    std::shared_ptr<ServiceControl> service,
    StatusLed *mqttLed);

void runSetupMode(
    std::shared_ptr<WebServer> webServer,
    std::shared_ptr<ServiceControl> service);

bool checkButtonHeld(int inputPin, StatusLed *led);

void doPollingSleep(uint ms)
{
#if PICO_CYW43_ARCH_POLL
    auto wakeTime = make_timeout_time_ms(ms);
    //auto totalPollTime = 0ll;
    //auto totalWaitTime = 0ll;
    do
    {
        //auto startPoll = get_absolute_time();
        cyw43_arch_poll();
        //auto endPoll = get_absolute_time();
        //totalPollTime += absolute_time_diff_us(startPoll, endPoll);
        cyw43_arch_wait_for_work_until(wakeTime);
        //totalWaitTime += absolute_time_diff_us(endPoll, get_absolute_time());
    } while(absolute_time_diff_us(get_absolute_time(), wakeTime) > 0);

    //if(totalPollTime > totalWaitTime)
    //{
    //    DBG_PRINT("Long poll: %u/%u ms\n", (int)(totalPollTime/1000), ms);
    //}
    
#else
    // We're using background IRQ callbacks from LWIP, so we can just sleep peacfully...
    sleep_ms(ms);
#endif
}

int main()
{
#ifndef NDEBUG
    stdio_init_all();
#endif

    // Set up pins for the hard buttons
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

    // LED timers need the async context set up to work
    if (cyw43_arch_init()) {
        sleep_ms(6000);
        DBG_PUT("Pico init failed");
        return -1;
    }

    StatusLed redLed(PIN_LED_R);
    StatusLed greenLed(PIN_LED_G);
    StatusLed blueLed(PIN_LED_B);

    // Pointless startup cycle, to give me enough time to start putty
    redLed.Pulse(256, 768, 128);
    doPollingSleep(2000);
    greenLed.Pulse(128, 512, 128);
    doPollingSleep(2000);
    blueLed.Pulse(128, 512, 128);
    doPollingSleep(2000);

    redLed.TurnOff();
    greenLed.TurnOff();
    blueLed.TurnOff();

    DBG_PUT("Starting the Radio");
    auto radio = std::make_shared<RFM69Radio>(SPI_PORT, PIN_CS_RADIO, PIN_RESET_RADIO);

    // We'll run radio commands from the core 1 thread
    auto commandQueue = std::make_shared<RadioCommandQueue>(radio, &blueLed);
    DBG_PUT("Starting worker thread...");
    commandQueue->Start();

    // Store flash settings at the very top of Flash memory
    // ALL STORED DATA WILL BE LOST IF THESE ARE CHANGED
    const uint32_t StorageSize = STORAGE_SECTORS * FLASH_SECTOR_SIZE;

    auto config = std::make_shared<DeviceConfig>(StorageSize, STORAGE_BLOCK_SIZE);
    auto wifiConfig = checkConfig(config, &redLed);
    if(wifiConfig == nullptr)
    {
        redLed.SetLevel(1024);
        blueLed.SetLevel(1024);
        return -1;
    }

    auto apMode = !wifiConfig->ssid[0];

    auto service = std::make_shared<ServiceControl>();

    if(checkButtonHeld(PIN_WIFI, &greenLed))
    {
        DBG_PUT("Starting in WIFI Setup mode, as button WIFI button is pressed");
        apMode = true;
    }

    DBG_PUT("Starting the WiFi Scanner...");
    auto wifiLed = apMode?&greenLed:&redLed;
    auto wifiConnection = std::make_shared<WifiConnection>(config, apMode, wifiLed);

    // Do an initial WiFi scan before entering AP mode. We don't seem to be able
    // to scan once in AP mode.
    auto wifiScanner = std::make_shared<WifiScanner>(apMode);
    wifiScanner->WaitForScan();

    // Now connect to WiFi or Enable AP mode
    DBG_PRINT("Enabling WiFi in %s mode...\n", apMode ? "AP" : "Client");
    wifiConnection->Start();

    DBG_PUT("Starting the web interface...");
    auto webServer = std::make_shared<WebServer>(config, wifiConnection, wifiLed);
    webServer->Start();
    auto configService = std::make_shared<ConfigService>(config, webServer, wifiScanner, service);

    if(apMode)
    {
        runSetupMode(webServer, service);
    }
    else
    {
        runServiceMode(webServer, config, wifiConnection, commandQueue, service, &greenLed);
    }

    DBG_PUT("Restarting now!");
    doPollingSleep(1000);

    if(service->IsFirmwareUpdateRequested())
    {
        reset_usb_boot(0,0);
    }
    else
    {
        watchdog_reboot(0,0,500);
    }
    doPollingSleep(5000);
    DBG_PUT("ERM!!! Why no reboot?");

}

bool checkButtonHeld(int inputPin, StatusLed *led)
{
    int a = 0;
    for(a = 0; !gpio_get(inputPin) && a < 100; a++)
    {
        if(a == 0)
            led->Pulse(1024, 2048, 256);
        doPollingSleep(50);
    }
    led->TurnOff();
    return a == 100;
}

const WifiConfig *checkConfig(
    std::shared_ptr<DeviceConfig> config,
    StatusLed *redLed)
{
    DBG_PUT("Checking Device config...");
    
    auto needReset = config->GetWifiConfig() == nullptr;
    if(needReset)
        DBG_PUT("WiFi Config not found.");
    else
    {
        needReset = checkButtonHeld(PIN_RESET, redLed);
    }

    if(needReset)
    {
        DBG_PUT("Resetting all settings!");
        config->HardReset();
    }


    auto wifiConfig = config->GetWifiConfig();
    if(wifiConfig == nullptr ||
        config->GetMqttConfig() == nullptr)
    {
        DBG_PUT("Settings could not be read back. Error!\n");
        return nullptr;
    }
    return wifiConfig;
}

void runServiceMode(
    std::shared_ptr<WebServer> webServer,
    std::shared_ptr<DeviceConfig> config,
    std::shared_ptr<WifiConnection> wifiConnection,
    std::shared_ptr<RadioCommandQueue> commandQueue,
    std::shared_ptr<ServiceControl> service,
    StatusLed *mqttLed)
{

    auto mqttClient = std::make_shared<MqttClient>(config, wifiConnection, "pico_somfy/status", "online", "offline", mqttLed);

    mqttClient->Start();
    // Subscribe by wildcard to reduce overhead
    mqttClient->SubscribeTopic("pico_somfy/blinds/+/cmd");
    mqttClient->SubscribeTopic("pico_somfy/blinds/+/pos");
    mqttClient->SubscribeTopic("pico_somfy/remotes/+/cmd");

    auto blinds = std::make_shared<Blinds>(config, mqttClient, webServer);
    auto remotes = std::make_shared<SomfyRemotes>(config, blinds, webServer, commandQueue, mqttClient);
    blinds->Initialize(remotes);

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

    auto asyncContext = cyw43_arch_async_context();

    DBG_PUT("Entering main loop...");
    bool resetPushed = false;
    while(!service->IsStopRequested()) {

        doPollingSleep(250);

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

        if(!gpio_get(PIN_RESET))
        {
            if(resetPushed)
            {
                DBG_PUT("Reset pushed!");
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
            // TODO: Reset WiFi or something in this case?
            DBG_PUT("WIFI pushed");
        }

    }

    republishTimer.ResetTimer(0);

    DBG_PUT("Waiting for the command queue to clear...");
    commandQueue->Shutdown();


    DBG_PUT("Saving any unsaved state.\n");
    remotes->SaveRemoteState();
    blinds->SaveBlindState(true);

}

void runSetupMode(
    std::shared_ptr<WebServer> webServer,
    std::shared_ptr<ServiceControl> service)
{
    ServiceStatus statusApi(webServer, nullptr, true);

    DBG_PUT("Entering setup loop...");
    auto onOff = false;
    while(!service->IsStopRequested()) {

        doPollingSleep(1000);

        if(!gpio_get(PIN_RESET))
        {
            DBG_PUT("Reset pushed");
            service->StopService();
        }
        if(!gpio_get(PIN_WIFI))
            DBG_PUT("WIFI pushed");
    }

}