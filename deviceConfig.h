

#pragma once

class BlockStorage;

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

class DeviceConfig
{
    public:
        DeviceConfig(BlockStorage &storage);

        const WifiConfig *GetWifiConfig();
        void SaveWifiConfig(const WifiConfig *wifiConfig);

        const MqttConfig *GetMqttConfig();
        void SaveMqttConfig(const MqttConfig *mqttConfig);

        void HardReset();

    private:
        BlockStorage &_storage;
        
};
