

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "pico/flash.h"

#include "mqttClient.h"
#include "deviceConfig.h"
#include "iwifiConnection.h"

MqttClient::MqttClient(DeviceConfig &config, IWifiConnection &wifi, const char *statusTopic, const char *onlinePayload, const char *offlinePayload)
: _config(config),
  _wifi(wifi),
  _statusTopic(statusTopic),
  _onlinePayload(onlinePayload),
  _offlinePayload(offlinePayload),
  _payload(nullptr)
{
}

void MqttClient::Start()
{
    auto mqttConfig = _config.GetMqttConfig();
    _client = mqtt_client_new();

    if(!strnlen(mqttConfig->brokerAddress, sizeof(mqttConfig->brokerAddress)))
        return;

    _watchdogTimer = std::make_unique<ScheduledTimer>([this]() { return MqttWatchdog(); }, 5000);

}

void MqttClient::DoConnect()
{
    if(!_wifi.IsConnected())
    {
        puts("Not connecting to MQTT, because WiFi is not connected\n");
        return;
    }

    auto mqttConfig = _config.GetMqttConfig();
    printf("Connecting to MQTT server at %s:%d\n", mqttConfig->brokerAddress, mqttConfig->port);
    mqtt_connect_client_info_t ci;
    memset(&ci, 0, sizeof(ci));
    err_t err;
    
    ci.client_id = "pico_somfy";
    if(mqttConfig->username[0])
        ci.client_user = mqttConfig->username;
    if(mqttConfig->password[0])
        ci.client_pass = mqttConfig->password;
    ci.will_msg = _offlinePayload;
    ci.will_retain = true;
    ci.will_topic = _statusTopic;
    ci.keep_alive = 50;

    ip_addr_t brokerAddress;
    ipaddr_aton(mqttConfig->brokerAddress, &brokerAddress);

    printf("Addr: %08x\n", brokerAddress.addr);
    
    err = mqtt_client_connect(_client, &brokerAddress, mqttConfig->port, ConnectionCallbackEntry, this, &ci);
    
    if(err != ERR_OK) {
        printf("Mqtt connection failure: %d\n", err);
    }    
    else
    {
        printf("Mqtt connection starting...\n");
    }
}

uint32_t MqttClient::MqttWatchdog()
{
    if(!IsConnected())
    {
        DoConnect();
    }

    // Restart timer
    return 60000;
}

bool MqttClient::IsConnected()
{
    return mqtt_client_is_connected(_client);
}


void MqttClient::Subscribe(const char *topic, SubscribeFunc &&callback)
{
    _subscriptions.insert({topic, callback});

    if(IsConnected())
    {
        mqtt_subscribe(_client, topic, 0, SubscriptionRequestCallbackEntry, this);
    }
}


void MqttClient::Unsubscribe(const char *topic)
{
    _subscriptions.erase(topic);
    if(IsConnected())
    {
        mqtt_unsubscribe(_client, topic, SubscriptionRequestCallbackEntry, this);
    }
}

bool MqttClient::Publish(const char *topic, const uint8_t *payload, uint32_t length)
{
    if(!IsConnected())
        return false;
    auto result = mqtt_publish(_client, topic, payload, length, 0, true, PublishCallbackEntry, this);
    return result == ERR_OK;
}


void MqttClient::ConnectionCallback(mqtt_connection_status_t status)
{
    switch(status)
    {
        case MQTT_CONNECT_ACCEPTED:
            puts("Mqtt client is connected");
            Publish(_statusTopic, (const uint8_t *)_onlinePayload, strlen(_onlinePayload));
            DoSubscribe();
            break;
        
        default:
            printf("Mqtt client failed to connect (%d)\n", status);
            break;

        case MQTT_CONNECT_DISCONNECTED:
            puts("Mqtt client disconnected\n");
            break;
    }
}

void MqttClient::DoSubscribe()
{
    for(auto const &sub : _subscriptions)
    {
        mqtt_set_inpub_callback(_client, IncomingPublishCallbackEntry, IncomingPayloadCallbackEntry, this);

        auto topic = sub.first.c_str();
        mqtt_request_cb_t cb;
        
        mqtt_subscribe(_client, topic, 0, SubscriptionRequestCallbackEntry, this);
    }
}

void MqttClient::SubscriptionResultCallback(err_t result)
{
    printf("Subscribe result: %d\n", result);
}

void MqttClient::PublishCallback(err_t result)
{
    printf("Publish result: %d\n", result);
}

void MqttClient::IncomingPublishCallback(const char *topic, u32_t tot_len)
{
    auto sub = _subscriptions.find(topic);
    if(sub == _subscriptions.end())
    {
        printf("Recieved a message for %s with no subscription\n", topic);
        if(_payload)
        {
            delete [] _payload;
            _payload = nullptr;
        }
        return;
    }

    if(tot_len > 2048)
    {
        printf("Recieved a message of %d bytes, greater than maximum buffer size\n", tot_len);
        return;
    }

    if(_payload)
    {
        puts("Unexpected: Overwriting previous payload buffer\n");
        delete [] _payload;
        _payload = nullptr;
    }

    _currentTopic = sub->first;
    _payload = new uint8_t[tot_len];
    _payloadLength = tot_len;
    _payloadReceived = 0;
}

void MqttClient::IncomingPayloadCallback(const u8_t *data, u16_t len, u8_t flags)
{
    if(_payload == nullptr)
    {
        printf("Ignoring %d bytes of unexpected payload\n", len);
    }

    auto remaining = _payloadLength - _payloadReceived;
    if(len > remaining)
    {
        puts("Got more payload than we expected\n");
        delete [] _payload;
        _payload = nullptr;
        return;
    }

    memcpy(_payload + _payloadReceived, data, len);
    _payloadReceived += len;

    if(flags & MQTT_DATA_FLAG_LAST)
    {
        // Call the subscriber...
        _subscriptions[_currentTopic](_payload, _payloadLength);
        delete [] _payload;
        _payload = nullptr;
    }
}


void MqttClient::ConnectionCallbackEntry(mqtt_client_t *client, void *arg, mqtt_connection_status_t status)
{
    auto pThis = (MqttClient *)arg;
    pThis->ConnectionCallback(status);
}

void MqttClient::SubscriptionRequestCallbackEntry(void *arg, err_t result)
{
    auto pthis = (MqttClient *)arg;
    pthis->SubscriptionResultCallback(result);
}


void MqttClient::IncomingPublishCallbackEntry(void *arg, const char *topic, u32_t tot_len)
{
    auto pthis = (MqttClient *)arg;
    pthis->IncomingPublishCallback(topic, tot_len);
}

void MqttClient::IncomingPayloadCallbackEntry(void *arg, const u8_t *data, u16_t len, u8_t flags)
{
    auto pthis = (MqttClient *)arg;
    pthis->IncomingPayloadCallback(data, len, flags);
}

void MqttClient::PublishCallbackEntry(void *arg, err_t result)
{
    auto pthis = (MqttClient *)arg;
    pthis->PublishCallback(result);
}