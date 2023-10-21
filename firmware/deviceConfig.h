#pragma once

#include "blockStorage.h"

#define SAVE_DELAY 120000


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
    // Primary remote for controlling this blind
    uint32_t remoteId;
};

struct RemoteConfig
{
    char remoteName[48];
    uint32_t remoteId;
    uint16_t rollingCode;
    uint16_t blindCount;
    // The blinds we at least think are associated with this remote
    uint16_t blinds[32];
};

bool operator==(const BlindConfig &left, const BlindConfig &right);

bool operator==(const RemoteConfig &left, const RemoteConfig &right);

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

        /// @brief Get the IDs of all the registered remotes
        /// @param count [out] number of remotes registered
        /// @return For reasons, only the lower 16 bits of the ID is returned. You need to use a fixed upper 8 bits
        const uint16_t *GetRemoteIds(uint32_t *count);
        void SaveRemoteIds(const uint16_t *blindIds, uint32_t count);

        const BlindConfig *GetBlindConfig(uint16_t blindId);
        void SaveBlindConfig(uint16_t blindId, const BlindConfig *blindConfig);
        void DeleteBlindConfig(uint16_t blindId);

        const RemoteConfig *GetRemoteConfig(uint32_t remoteId);
        void SaveRemoteConfig(uint32_t remoteId, const RemoteConfig *remoteConfig);
        void DeleteRemoteConfig(uint32_t remoteId);

        void HardReset();

    private:
        DeviceConfig(const DeviceConfig &) = delete;

        void SaveIdList(uint32_t header, const uint16_t *ids, uint32_t count);
        const uint16_t *GetIdList(uint32_t header, uint32_t *count);

        BlockStorage _storage;        
};
