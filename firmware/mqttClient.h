// Copyright (c) 2023 Mark Godwin.
// SPDX-License-Identifier: MIT

#pragma once

#include "lwip/apps/mqtt.h"
#include <functional>
#include <map>
#include <set>
#include <memory>
#include "scheduler.h"

class DeviceConfig;
class IWifiConnection;
struct async_context;
class StatusLed;

typedef std::function<void(const uint8_t *, uint32_t)> SubscribeFunc;

/// @brief Mqtt client that does its best to stay connected
class MqttClient
{
    public:

        MqttClient(std::shared_ptr<DeviceConfig> config, std::shared_ptr<IWifiConnection> wifi, const char *statusTopic, const char *onlinePayload, const char *offlinePayload, StatusLed *statusLed);

        void Start();

        /// @brief True, if MQTT is configured
        bool IsEnabled() { return _client != nullptr; }

        /// @brief True if MQTT is enabled and connected.
        bool IsConnected();

        /// @brief Add a subscription to the client. Can be used with wildcards
        /// @param topic Topic to subscribe to, with wildcards if needed
        void SubscribeTopic(const char *topic);
        void UnsubscribeTopic(const char *topic);

        /// @brief Add a callback when a given topic is matched. The topic (or a matching wildcard) must have been subscribed already
        /// @param topic Topic to match (no wildcards!)
        /// @param callback Callback to call
        void AddTopicCallback(const char *topic, SubscribeFunc &&callback);
        void RemoveTopicCallback(const char *topic);

        bool Publish(const char *topic, const uint8_t *payload, uint32_t length, bool retain = true);

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

        StatusLed *_statusLed;
        std::unique_ptr<ScheduledTimer> _watchdogTimer;
        std::shared_ptr<IWifiConnection> _wifi;
        std::shared_ptr<DeviceConfig> _config;
        mqtt_client_t *_client;

        const char *_statusTopic;
        const char *_onlinePayload;
        const char *_offlinePayload;

        std::set<std::string> _subscribedTopics;
        std::map<std::string, SubscribeFunc> _topicCallbacks;
        std::string _currentTopic;
        uint8_t *_payload;
        uint32_t _payloadLength;
        uint32_t _payloadReceived;


};

template<typename ... Args>
std::string string_format( const std::string& format, Args ... args )
{
    int size_s = std::snprintf( nullptr, 0, format.c_str(), args ... ) + 1; // Extra space for '\0'
    if( size_s <= 0 )
        return "##";
    auto size = static_cast<size_t>( size_s );
    std::unique_ptr<char[]> buf( new char[ size ] );
    std::snprintf( buf.get(), size, format.c_str(), args ... );
    return std::string( buf.get(), buf.get() + size - 1 ); // We don't want the '\0' inside
}

/// @brief Helper class for managing a subscription lifetime
class MqttSubscription
{
    public:
        MqttSubscription(std::shared_ptr<MqttClient> client, std::string topic, SubscribeFunc &&callback, bool doSubscribe = false)
        :   _client(client),
             _topic(std::move(topic)),
             _subscribed(doSubscribe)
        {
            _client->AddTopicCallback(_topic.c_str(), std::forward<SubscribeFunc>(callback));
            // Normally, we expect the topic would have been subscribed by wildcard
            if(_subscribed)
                client->SubscribeTopic(topic.c_str());
            
        }

        MqttSubscription(MqttSubscription &&other)
        :   _client(std::move(other._client)),
            _topic(std::move(other._topic)),
            _subscribed(other._subscribed)
        {
        }


        ~MqttSubscription()
        {
            if(_client)
            {
                if(_subscribed)
                    _client->UnsubscribeTopic(_topic.c_str());
                _client->RemoveTopicCallback(_topic.c_str());
            }
        }
       
    private:
        MqttSubscription(const MqttSubscription &) = delete;
        std::shared_ptr<MqttClient> _client;
        std::string _topic;
        bool _subscribed;
};
