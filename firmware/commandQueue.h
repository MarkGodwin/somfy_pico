// Copyright (c) 2023 Mark Godwin.
// SPDX-License-Identifier: MIT

#pragma once

enum SomfyButton : int;

#include "pico/util/queue.h"
#include <memory>

class RFM69Radio;
class StatusLed;

struct SomfyCommand
{
    uint32_t remoteId;
    uint16_t rollingCode;
    uint16_t repeat;
    SomfyButton button;
};

struct RecvCommand
{
    SomfyCommand command;
    absolute_time_t lastMsgTime;
};

#define MAX_RECV_QUEUE 8

/// @brief Queue for executing radio commands
/// @remarks Because radio commands take a while, and need to be executed with precise timing (and for fun/overkill) we'll run the commands from the pico's second thread
class RadioCommandQueue
{
public:
    RadioCommandQueue(std::shared_ptr<RFM69Radio> radio, StatusLed *led);

    bool QueueCommand(SomfyCommand command);

    // Read commands from other remotes recieved over the airwaves
    bool ReadCommand(SomfyCommand *command);

    void Shutdown();

    /// @brief Runs the queue processing until shut down
    void Start();


private:
    void QueueReceive();

    void Worker();
    void ExecuteCommand(uint32_t remoteId, uint16_t rollingCode, SomfyButton button, uint16_t repeat);
    void ReceiveCommand();

    std::shared_ptr<RFM69Radio> _radio;
    StatusLed *_led;
    queue_t _queue;

    int _recvWrite;
    int _recvRead;
    volatile int _recvCount;
    int _lockNum;
    spin_lock_t *_recvLock;
    RecvCommand _receivedCommands[MAX_RECV_QUEUE];



};
