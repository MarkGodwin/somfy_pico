#pragma once

#include "lwip/apps/mqtt.h"
#include <functional>
#include <map>
#include <memory>
#include "scheduler.h"

class DeviceConfig;
class IWifiConnection;
struct async_context;

typedef std::function<void(const uint8_t *, uint32_t)> SubscribeFunc;

/// @brief Mqtt client that does its best to stay connected
class MqttClient
{
    public:

        MqttClient(DeviceConfig &config, IWifiConnection &wifi, const char *statusTopic, const char *onlinePayload, const char *offlinePayload);

        void Start();

        bool IsConnected();

        void Subscribe(const char *topic, SubscribeFunc &&callback);
        void Unsubscribe(const char *topic);

        bool Publish(const char *topic, const uint8_t *payload, uint32_t length);

    private:
        static void ConnectionCallbackEntry(mqtt_client_t *client, void *arg, mqtt_connection_status_t status);
        void ConnectionCallback(mqtt_connection_status_t status);

        void DoConnect();
        void DoSubscribe();

        uint32_t MqttWatchdog();

        static void SubscriptionRequestCallbackEntry(void *arg, err_t result);
        void SubscriptionResultCallback(err_t result);

        static void IncomingPublishCallbackEntry(void *arg, const char *topic, u32_t tot_len);
        void IncomingPublishCallback(const char *topic, u32_t tot_len);

        static void IncomingPayloadCallbackEntry(void *arg, const u8_t *data, u16_t len, u8_t flags);
        void IncomingPayloadCallback(const u8_t *data, u16_t len, u8_t flags);

        static void PublishCallbackEntry(void *arg, err_t result);
        void PublishCallback(err_t result);

        std::unique_ptr<ScheduledTimer> _watchdogTimer;
        IWifiConnection &_wifi;
        DeviceConfig &_config;
        mqtt_client_t *_client;

        const char *_statusTopic;
        const char *_onlinePayload;
        const char *_offlinePayload;

        std::map<std::string, SubscribeFunc> _subscriptions;
        std::string _currentTopic;
        uint8_t *_payload;
        uint32_t _payloadLength;
        uint32_t _payloadReceived;


};
