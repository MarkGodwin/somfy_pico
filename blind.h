#pragma once


#include "mqttClient.h"
#include "webInterface.h"
#include <memory>


class SomfyRemote;
enum SomfyButton : int;
struct BlindConfig;

class Blind
{
    public:
        Blind(uint32_t blindId,
            const BlindConfig *config,
            std::shared_ptr<SomfyRemote> remote,
            std::shared_ptr<MqttClient> mqttClient,
            std::shared_ptr<WebServer> webServer);

        ~Blind();

        void GetConfig(BlindConfig *config);

        /// @brief Command the blind to move to a specific position, using the primary remote
        /// @param position New position for the blind
        void GoToPosition(int position);
        void GoUp();
        void GoDown();
        void Stop();
        void GoToMyPosition();

        /// @brief Called if buttons on a remote linked to this blind are pressed
        void ButtonsPressed(SomfyButton button);

        /// @brief Saves the current position as the My position
        void SaveMyPosition();

        int GetTargetPosition(int position) { return _targetPosition; }
        int GetIntermediatePosition(int position) { return _intermediatePosition; }

        /// @brief True, if we think the blind is moving
        bool IsInMotion() { return _motionDirection != 0; }

        /// @brief Called periodically for blinds in motion to update their guess of their actual position.
        void UpdatePosition();

        void OnCommand(const uint8_t *payload, uint32_t length);
        void OnSetPosition(const uint8_t *payload, uint32_t length);

    private:
        Blind(const Blind&) = delete;


        uint32_t _blindId;

        int _motionDirection;
        int _intermediatePosition; 
        int _targetPosition;      // Position of the blind, 100 for open, 0 for closed.
        int _favouritePosition;

        std::shared_ptr<MqttClient> _mqttClient;
        std::shared_ptr<WebServer> _webServer;
        std::shared_ptr<SomfyRemote> _remote;
        Subscription _cmdSubscription;
        Subscription _posSubscription;


};

