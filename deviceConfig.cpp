
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/flash.h"
#include "pico/malloc.h"

#include "deviceConfig.h"
#include "blockStorage.h"

static const uint32_t wifiConfigMagic = 0x19841984;
static const uint32_t mqttConfigMagic = 0x19841985;
static const uint32_t blindsConfigMagic = 0x19841986;
static const uint32_t remotesConfigMagic = 0x19841987;
static const uint32_t blindConfigMagic = 0x19850000;
static const uint32_t remoteConfigMagic = 0x19860000;

DeviceConfig::DeviceConfig(uint32_t storageSize, uint32_t blockSize)
:   _storage(PICO_FLASH_SIZE_BYTES - storageSize, storageSize, blockSize)
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

void DeviceConfig::SaveBlindIds(const uint16_t *blindIds, uint32_t count)
{
    SaveIdList(blindsConfigMagic, blindIds, count);
}

const uint16_t *DeviceConfig::GetBlindIds(uint32_t *count)
{
    return GetIdList(blindsConfigMagic, count);
}

void DeviceConfig::SaveRemoteIds(const uint16_t *remoteIds, uint32_t count)
{
    SaveIdList(remotesConfigMagic, remoteIds, count);
}

const BlindConfig *DeviceConfig::GetBlindConfig(uint16_t blindId)
{
    return (BlindConfig *)_storage.GetBlock(blindConfigMagic | blindId);
}

void DeviceConfig::SaveBlindConfig(uint16_t blindId, const BlindConfig *blindConfig)
{
    // Don't save if there are no changes, to avoid flash wear
    auto existingCfg = GetBlindConfig(blindId);
    if(existingCfg &&
        *blindConfig == *existingCfg)
    {
        return;
    }
    _storage.SaveBlock(blindConfigMagic | blindId, (const uint8_t *)blindConfig, sizeof(blindConfig));
}

const RemoteConfig *DeviceConfig::GetRemoteConfig(uint32_t remoteId)
{
    return (RemoteConfig *)_storage.GetBlock(remoteConfigMagic | (remoteId & 0xFFFF));
}

void DeviceConfig::SaveRemoteConfig(uint32_t remoteId, const RemoteConfig *remoteConfig)
{
    // Don't save if there are no changes, to avoid flash wear
    auto existingCfg = GetRemoteConfig(remoteId);
    if(existingCfg &&
        *remoteConfig == *existingCfg)
    {
        return;
    }
    _storage.SaveBlock(remoteConfigMagic | (remoteId & 0xFFFF), (const uint8_t *)remoteConfig, sizeof(remoteConfig));
}

const uint16_t *DeviceConfig::GetRemoteIds(uint32_t *count)
{
    return GetIdList(remotesConfigMagic, count);
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
        strcpy(mcfg.brokerAddress, "");
        mcfg.port = 1883;
        puts("Saving Mqtt config\n");
        SaveMqttConfig(&mcfg);

}

void DeviceConfig::SaveIdList(uint32_t header, const uint16_t *ids, uint32_t count)
{
    size_t bytes = sizeof(count) + sizeof(uint16_t) * count;
    if(bytes > _storage.BlockSize())
        puts("Too many ids to fit in the block. FAIL\n");

    auto buf = (uint8_t *)malloc(bytes);
    if(buf == nullptr)
    {
        printf("Failed to allocate a buffer of %d bytes to save ID list\n", bytes);
        return;
    }
    memcpy(buf, &count, sizeof(count));
    memcpy(buf + sizeof(count), ids, sizeof(uint16_t) * count);
    _storage.SaveBlock(header, buf, bytes);
    free(buf);
}

const uint16_t *DeviceConfig::GetIdList(uint32_t header, uint32_t *count)
{
    auto block = (const uint32_t *)_storage.GetBlock(header);
    if(block == nullptr)
    {
        *count = 0;
        return nullptr;
    }
    *count = *block;
    return (const uint16_t *)(block + 1);
}

bool operator==(const RemoteConfig &left, const RemoteConfig &right)
{
    return left.remoteId == right.remoteId &&
        left.rollingCode == right.rollingCode &&
        left.blindCount == right.blindCount &&
        !memcmp(left.blinds, right.blinds, sizeof(left.blinds[0]) * left.blindCount) &&
        !strcmp(left.remoteName, right.remoteName);
}

bool operator==(const BlindConfig &left, const BlindConfig &right)
{
    return !strcmp(left.blindName, right.blindName) &&
        left.currentPosition == right.currentPosition &&
        left.myPosition == right.myPosition &&
        left.openTime == right.openTime &&
        left.closeTime == right.closeTime &&
        left.remoteId == right.remoteId;
}
