#pragma once

#include "blockStorage.h"

struct WifiConfig
{
    char ssid[36];
    char password[64];
};

struct MqttConfig
{
    char brokerAddress[16];
    uint16_t port;
    char username[32];
    char password[64];
    char topic[128];
};

struct BlindConfig
{
    char blindName[48];
    int currentPosition;
    int myPosition;
    int openTime;
    int closeTime;
    uint32_t remoteId;
};

class DeviceConfig
{
    public:
        DeviceConfig(uint32_t storageSize, uint32_t blockSize);

        const WifiConfig *GetWifiConfig();
        void SaveWifiConfig(const WifiConfig *wifiConfig);

        const MqttConfig *GetMqttConfig();
        void SaveMqttConfig(const MqttConfig *mqttConfig);

        const uint16_t *GetBlindIds(uint32_t *count);
        void SaveBlindIds(const uint16_t *blindIds, uint32_t count);
        const uint16_t *GetRemoteIds(uint32_t *count);
        void SaveRemoteIds(const uint16_t *blindIds, uint32_t count);

        const BlindConfig *GetBlindConfig(uint16_t blindId);
        void SaveBlindConfig(uint16_t blindId, const BlindConfig *blindConfig);

        void HardReset();

    private:
        DeviceConfig(const DeviceConfig &) = delete;

        void SaveIdList(uint32_t header, const uint16_t *ids, uint32_t count);
        const uint16_t *GetIdList(uint32_t header, uint32_t *count);

        BlockStorage _storage;        
};
