#include "pico/stdlib.h"
#include "serviceStatus.h"
#include "mqttClient.h"

int16_t OutputBool(bool value, char *pcInsert)
{
    if(value)
    {
        memcpy(pcInsert, "true", 4);
        return 4;
    }
    memcpy(pcInsert,"false", 5);
    return 5;
}

ServiceStatus::ServiceStatus(std::shared_ptr<WebServer> webServer, std::shared_ptr<MqttClient> mqttClient, bool apMode)
:   _mqttClient(mqttClient),
    _apMode(apMode),
    _apModeSub(webServer, "apmode", [this] (char *pcInsert, int iInsertLen, uint16_t tagPart, uint16_t *nextPart) { return OutputBool(_apMode, pcInsert); }),
    _mqttSub(webServer, "mqttconn", [this] (char *pcInsert, int iInsertLen, uint16_t tagPart, uint16_t *nextPart) { return OutputBool(_mqttClient->IsConnected(), pcInsert); })
{
    
}
