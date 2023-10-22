
#pragma once

enum SomfyButton : int;

#include "pico/util/queue.h"
#include <memory>

class RFM69Radio;
class StatusLed;

/// @brief Queue for executing radio commands
/// @remarks Because radio commands take a while, and need to be executed with precise timing (and for fun/overkill) we'll run the commands from the pico's second thread
class RadioCommandQueue
{
public:
    RadioCommandQueue(std::shared_ptr<RFM69Radio> radio, StatusLed *led);

    bool QueueCommand(uint32_t remoteId, uint16_t rollingCode, SomfyButton button, uint16_t repeat);

    void Shutdown();

    /// @brief Runs the queue processing until shut down
    void Start();

private:
    void Worker();
    void ExecuteCommand(uint32_t remoteId, uint16_t rollingCode, SomfyButton button, uint16_t repeat);

    std::shared_ptr<RFM69Radio> _radio;
    StatusLed *_led;
    queue_t _queue;

};
