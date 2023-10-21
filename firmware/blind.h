#pragma once


#include "mqttClient.h"
#include <memory>
#include <scheduler.h>

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
            std::shared_ptr<MqttClient> mqttClient,
            std::shared_ptr<DeviceConfig> config);

        ~Blind();

        // Config API
        const std::string &GetName() { return _name; }
        void UpdateConfig(std::string name, int openTime, int closeTime);
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
        int GetIntermediatePosition() { return (int)_intermediatePosition; }

        /// @brief 1 for up, -1 for down (if we think the blind is moving)
        bool GetMotionDirection() { return _motionDirection; }

        void OnCommand(const uint8_t *payload, uint32_t length);
        void OnSetPosition(const uint8_t *payload, uint32_t length);

        bool NeedsPublish() { return _needsPublish; }
        void TriggerPublishDiscovery() {
            _needsPublish = true;
            _discoveryWorker.ScheduleWork();
        }

        void SaveConfig(bool force = false);

    private:
        Blind(const Blind&) = delete;

        /// @brief Called periodically for blinds in motion to update their guess of their actual position.
        uint32_t UpdatePosition();
        void PublishPosition();
        void PublishDiscovery();

        uint16_t _blindId;
        bool _isDirty;  // True if save is needed
        bool _needsPublish;
        std::string _name;
        int _openTime;
        int _closeTime;

        absolute_time_t _lastTick;
        int _motionDirection;

        float _intermediatePosition;
        int _targetPosition;      // Position of the blind, 100 for open, 0 for closed.
        int _favouritePosition;

        std::shared_ptr<MqttClient> _mqttClient;
        std::shared_ptr<SomfyRemote> _remote;
        std::shared_ptr<DeviceConfig> _config;
        MqttSubscription _cmdSubscription;
        MqttSubscription _posSubscription;
        ScheduledTimer _refreshTimer;
        PendingWorker _discoveryWorker;
};

