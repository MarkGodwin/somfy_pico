#pragma once

#include <memory>
#include "webServer.h"

class MqttClient;

class ServiceStatus
{
public:
    ServiceStatus(std::shared_ptr<WebServer> webServer, std::shared_ptr<MqttClient> mqttClient, bool apMode);

private:

    SsiSubscription _apModeSub;
    SsiSubscription _mqttSub;

    std::shared_ptr<MqttClient> _mqttClient;
    bool _apMode;
};
