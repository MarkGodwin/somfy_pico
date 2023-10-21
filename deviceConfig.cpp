
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/flash.h"

#include "deviceConfig.h"
#include "blockStorage.h"

static const uint32_t wifiConfigMagic = 0x19841984;
static const uint32_t mqttConfigMagic = 0x19841985;

DeviceConfig::DeviceConfig(BlockStorage &storage)
:   _storage(storage)
{
}

const WifiConfig * DeviceConfig::GetWifiConfig()
{
    auto creds = (const WifiConfig *)_storage.GetBlock(wifiConfigMagic);
    return creds;
}

void DeviceConfig::SaveWifiConfig(const WifiConfig *config)
{
    _storage.SaveBlock(wifiConfigMagic, (const uint8_t *)config, sizeof(WifiConfig));
}

const MqttConfig *DeviceConfig::GetMqttConfig()
{
    auto creds = (const MqttConfig *)_storage.GetBlock(mqttConfigMagic);
    return creds;
}

void DeviceConfig::SaveMqttConfig(const MqttConfig *mqttConfig)
{
    _storage.SaveBlock(mqttConfigMagic, (const uint8_t *)mqttConfig, sizeof(MqttConfig));
}

void DeviceConfig::HardReset()
{
        // The flash storage has no wifi config. Format it to ensure it is empty, and
        // store default device config
        _storage.Format();

        WifiConfig cfg;
        memset(&cfg, 0, sizeof(WifiConfig));
        puts("Saving WiFi config\n");
        SaveWifiConfig(&cfg);

        MqttConfig mcfg;
        memset(&mcfg, 0, sizeof(mcfg));
        strcpy(mcfg.brokerAddress, "192.168.0.1");
        mcfg.port = 1883;        
        puts("Saving Mqtt config\n");
        SaveMqttConfig(&mcfg);

}