
#include "pico/stdlib.h"

#include "blind.h"
#include "remote.h"
#include <string.h>
#include <math.h>
#include "deviceConfig.h"

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

Blind::Blind(
    uint32_t blindId,
    const BlindConfig *config,
    std::shared_ptr<SomfyRemote> remote,
    std::shared_ptr<MqttClient> mqttClient,
    std::shared_ptr<WebServer> webServer)
:   _blindId(blindId),
    _remote(remote),
    _mqttClient(mqttClient),
    _motionDirection(0),
    _cmdSubscription(mqttClient, string_format("pico_somfy/blinds/{%08x}/cmd", blindId),[this](const uint8_t *payload, uint32_t length) { OnCommand(payload, length); }),
    _posSubscription(mqttClient, string_format("pico_somfy/blinds/{%08x}/pos", blindId),[this](const uint8_t *payload, uint32_t length) { OnSetPosition(payload, length); })
{
    _targetPosition = config->currentPosition;
    _intermediatePosition = _targetPosition;

    printf("Blind ID %d: %s\n", _blindId, config->blindName);
}

Blind::~Blind()
{
}

void Blind::GoToPosition(int position)
{
    _targetPosition = position;
    if(_intermediatePosition > _targetPosition || _targetPosition == 0)
    {
        _remote->PressButtons(SomfyButton::Down, ShortPress);
        _motionDirection = -1;
        
    }
    if(_intermediatePosition < _targetPosition || _targetPosition == 100)
    {
        _remote->PressButtons(SomfyButton::Up, ShortPress);
        _motionDirection = 1;
    }
}

void Blind::GoUp()
{
    _remote->PressButtons(SomfyButton::Up, ShortPress);
    ButtonsPressed(SomfyButton::Up);
}

void Blind::GoDown()
{
    _remote->PressButtons(SomfyButton::Down, ShortPress);
    ButtonsPressed(SomfyButton::Down);
}

void Blind::Stop()
{
    if(_motionDirection)
    {
        _remote->PressButtons(SomfyButton::Down, ShortPress);
        ButtonsPressed(SomfyButton::My);
    }
}


void Blind::GoToMyPosition()
{
    if(_motionDirection)
        return;
    _targetPosition = _favouritePosition;
    _remote->PressButtons(SomfyButton::My, ShortPress);
    if(_intermediatePosition < _targetPosition)
        _motionDirection = 1;
    else if(_intermediatePosition > _targetPosition)
        _motionDirection = -1;
    else
        _motionDirection = 0;
}

void Blind::ButtonsPressed(SomfyButton button)
{
    if(button == SomfyButton::Up)
    {
        _targetPosition = 100;
        _motionDirection = 1;
    }
    if(button == SomfyButton::Down)
    {
        _targetPosition = 0;
        _motionDirection = -1;
    }
    if(button == SomfyButton::My)
    {
        if(_motionDirection)
        {
            _targetPosition = _intermediatePosition;
            _motionDirection = 0;
        }
        else
        {
            _targetPosition = _favouritePosition;
            _motionDirection = _targetPosition > _intermediatePosition ? 1 : -1;
        }
    }

}

void Blind::SaveMyPosition()
{
    if(!_motionDirection)
    {
        _remote->PressButtons(SomfyButton::My, LongPress);
        _favouritePosition = _intermediatePosition;
    }
}

void Blind::UpdatePosition()
{
    // Tick
}

void Blind::OnCommand(const uint8_t *payload, uint32_t length)
{
    if(length == 4 && !memcmp(payload, "open", 4))
        GoUp();
    else if(length == 5 && !memcmp(payload, "close", 5))
        GoDown();
    else if(length == 4 && !memcmp(payload, "stop", 4))
        Stop();
}

void Blind::OnSetPosition(const uint8_t *payload, uint32_t length)
{
    if(length > 31)
        return;
    char payloadstr[32];
    memcpy(payloadstr, payload, length);
    payloadstr[length] = 0;
    float position = 0;
    sscanf(payloadstr, "%f", &position);
    int pos = round(position);

    GoToPosition(pos);
}

