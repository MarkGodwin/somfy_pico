#pragma once


#include "mqttClient.h"
#include <memory>


class SomfyRemote;
enum SomfyButton : int;
struct BlindConfig;

class Blind
{
    public:
        Blind(
            uint16_t blindId,
            std::string name,
            int currentPosition,
            int favouritePosition,
            int openTime,
            int closeTime,
            std::shared_ptr<SomfyRemote> remote,
            std::shared_ptr<MqttClient> mqttClient);

        ~Blind();

        // Config API
        const std::string &GetName() { return _name; }
        void UpdateConfig(std::string name, int openTime, int closeTime) { _name = std::move(name); _openTime = openTime; _closeTime = closeTime; }
        int GetOpenTime() { return _openTime; }
        int GetCloseTime() { return _closeTime; }
        uint32_t GetRemoteId();
        int GetFavouritePosition() { return _favouritePosition; }

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

        int GetTargetPosition() { return _targetPosition; }
        int GetIntermediatePosition() { return _intermediatePosition; }

        /// @brief True, if we think the blind is moving
        bool IsInMotion() { return _motionDirection != 0; }

        /// @brief Called periodically for blinds in motion to update their guess of their actual position.
        void UpdatePosition();

        void OnCommand(const uint8_t *payload, uint32_t length);
        void OnSetPosition(const uint8_t *payload, uint32_t length);

    private:
        Blind(const Blind&) = delete;


        uint16_t _blindId;
        std::string _name;
        int _openTime;
        int _closeTime;

        int _motionDirection;
        int _intermediatePosition; 
        int _targetPosition;      // Position of the blind, 100 for open, 0 for closed.
        int _favouritePosition;

        std::shared_ptr<MqttClient> _mqttClient;
        std::shared_ptr<SomfyRemote> _remote;
        MqttSubscription _cmdSubscription;
        MqttSubscription _posSubscription;


};

