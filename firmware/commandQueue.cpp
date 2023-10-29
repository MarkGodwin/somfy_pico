// Copyright (c) 2023 Mark Godwin.
// SPDX-License-Identifier: MIT
// TODO: Make this a generic command queue

#include "picoSomfy.h"
#include "pico/multicore.h"
#include "pico/flash.h"
#include <string.h>
#include "commandQueue.h"
#include "radio.h"
#include "statusLed.h"
#include "remote.h"

struct CommandEntry
{
    uint32_t commandType;
    uint32_t remoteId;
    uint16_t rollingCode;
    uint16_t repeat;
    SomfyButton button;
};

class SpinLock
{
public:
    SpinLock(spin_lock_t *lock): _lock(lock)
    {
        _irq = spin_lock_blocking(_lock);
    }
    ~SpinLock()
    {
        spin_unlock(_lock, _irq);
    }

private:
    spin_lock_t *_lock;
    int32_t _irq;
};

RadioCommandQueue::RadioCommandQueue(std::shared_ptr<RFM69Radio> radio, StatusLed *led)
:   _radio(std::move(radio)),
    _led(led),
    _recvWrite(0),
    _recvRead(0),
    _recvCount(0)
{
    queue_init(&_queue, sizeof(CommandEntry), 16);
    _lockNum = spin_lock_claim_unused(true);
    _recvLock = spin_lock_init(_lockNum);
}

bool RadioCommandQueue::QueueCommand(SomfyCommand command)
{
    CommandEntry entry = { 
        commandType: 1,
        remoteId: command.remoteId,
        rollingCode: command.rollingCode,
        repeat: command.repeat,
        button: command.button
    };

    return queue_try_add(&_queue, &entry);
}

bool RadioCommandQueue::ReadCommand(SomfyCommand *command)
{
    auto now = get_absolute_time();

    // CRITICAL SECTION
    SpinLock scopedLock(_recvLock);

    if(!_recvCount)
        // Nothing is waiting to be picked up
        return false;

    if(absolute_time_diff_us(_receivedCommands[_recvRead].lastMsgTime, now) < 250000)
        // More of the command repeates might be incoming
        return false;

    command->remoteId = _receivedCommands[_recvRead].command.remoteId;
    command->rollingCode = _receivedCommands[_recvRead].command.rollingCode;
    command->repeat = _receivedCommands[_recvRead].command.repeat;
    command->button = _receivedCommands[_recvRead].command.button;
    _recvCount--;
    _recvRead = (_recvRead + 1) % (MAX_RECV_QUEUE);
    if(_recvCount == 0)
        _recvWrite = _recvRead;
    return true;
}

void RadioCommandQueue::Shutdown()
{
    CommandEntry entry = { 
        commandType: 0
    };

    queue_add_blocking(&_queue, &entry);

    // Wait until our Stop command is removed from the queue
    while(queue_get_level(&_queue))
    {
        sleep_ms(100);
    } 
}

void RadioCommandQueue::QueueReceive()
{
    CommandEntry entry = { 
        commandType: 2
    };

    queue_try_add(&_queue, &entry);
}

// HACK. But there can be only 1 anyway
static RadioCommandQueue *_thequeue = nullptr;

void RadioCommandQueue::Start()
{

    _thequeue = this;
    multicore_launch_core1([]() {
        // Make sure the other core can fiddle with flash without us screwing everything up
        if(!flash_safe_execute_core_init())
        {
            // Unsafe to continue this thread...
            DBG_PUT("Flash init failed");
            return;
        }

        _thequeue->Worker();

        // Don't actually end the command thread, as we want to write to flash
        // and if we stop this thread, the flash write may fail because it can't
        // synchronise
        while(true)
            sleep_ms(1000);

        //flash_safe_execute_core_deinit();
    });
}

void RadioCommandQueue::Worker()
{
    _led->SetLevel(512);
    _radio->Reset();
    DBG_PUT("Radio Reset complete!");
    _radio->Initialize();
    _radio->SetSymbolWidth(640);
    _radio->SetFrequency(433.42);
    auto fq = _radio->GetFrequency();
    DBG_PRINT("Radio frequency: %.3f\n", fq);
    auto br = _radio->GetBitRate();
    DBG_PRINT("BitRate: %dbps\n", br);
    auto ver = _radio->GetVersion();
    DBG_PRINT("Radio version: 0x%02x\n", ver);
    _led->SetLevel(0);

    while(true)
    {
        // Enter RX mode while we wait...
        // Wait for the end of any sync bytes (both initial and repeat ends the same way)
        // By offsetting the sync bytes by 1 bit, we get good reception... The sync pulses
        // from the genuine remotes don't match the main signal section symbol width exactly.
        uint8_t syncBytes[] = {0xE1, 0xE1, 0xFE};
        _radio->SetSyncBytes(syncBytes, sizeof(syncBytes));
        _radio->SetPacketFormat(true, 7);
        _radio->EnableReceive([]() {     _thequeue->QueueReceive(); } );

        CommandEntry entry;
        queue_remove_blocking(&_queue, &entry);
        _radio->Standby();

        switch(entry.commandType)
        {
            case 0:
                return;
            case 1:
                ExecuteCommand(entry.remoteId, entry.rollingCode, entry.button, entry.repeat);
                break;
            case 2:
                ReceiveCommand();
        }
    }
}

void RadioCommandQueue::ExecuteCommand(uint32_t remoteId, uint16_t rollingCode, SomfyButton button, uint16_t repeat)
{
    _led->SetLevel(1024);

    _radio->SetSyncBytes(NULL, 0);
    _radio->SetPacketFormat(false, 2);
    auto now = get_absolute_time();

    // Fog horn to wake up the blinds
    uint8_t fog[2];
    ::memset(fog, 0xFF, 2);
    _radio->TransmitPacket(fog, 16);
    _led->SetLevel(128);

    uint8_t payload[16] = {
        0x50,                       // "Key". Fixed...
        (uint8_t)(button << 4),
        (uint8_t)(rollingCode >> 8),
        (uint8_t)(rollingCode & 0xFF),
        (uint8_t)((remoteId >> 16) & 0xFF),
        (uint8_t)((remoteId >> 8) & 0xFF),
        (uint8_t)(remoteId & 0xFF),
    };
    memset(payload + 7, 0, 9);

    // Add the checksum
    uint8_t checksum = 0;
    for(auto a = 0; a < 7; a++)
        checksum ^= payload[a] ^ (payload[a] >> 4);
    payload[1] |= (checksum & 0x0F);

    // "Encrypt"
    for(auto a = 1; a < 7; a++)
        payload[a] ^= payload[a-1];

    const uint8_t syncBytes[] = { 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xFE };

    _radio->SetSyncBytes(syncBytes + 5, 3);
    _radio->SetPacketFormat(true, 7);
    auto packetStartTime = delayed_by_us(now, 29000); 
    while(absolute_time_diff_us(get_absolute_time(), packetStartTime) > 0);

    _led->SetLevel(1024);
    _radio->TransmitPacket(payload, sizeof(payload));
    _radio->SetSyncBytes(syncBytes, 8);
    packetStartTime = delayed_by_us(packetStartTime, 115000);
    _led->SetLevel(256);

    for(auto a = 0; a < repeat; a++)
    {
        while(absolute_time_diff_us(get_absolute_time(), packetStartTime) > 0);

        _led->SetLevel(1024);
        _radio->TransmitPacket(payload, sizeof(payload));
        _led->SetLevel(256);
        packetStartTime = delayed_by_us(packetStartTime, 139000);
    }
    _led->SetLevel(0);
}

void RadioCommandQueue::ReceiveCommand()
{
    uint8_t msg[7];
    _radio->ReceivePacket(msg, sizeof(msg));

    //printf("Packet recieved: %02x%02x%02x%02x%02x%02x%02x\n", msg[0], msg[1], msg[2], msg[3], msg[4], msg[5], msg[6]);

    // "Decrypt"...
    for(auto a = 6; a > 0; a--)
        msg[a] ^= msg[a-1];

    // Extract and zero out the checksum
    auto checksum = msg[1] & 0xf;
    msg[1] = msg[1] & 0xf0;

    uint8_t rcvChecksum = 0;
    for(auto a = 0; a < 7; a++)
        rcvChecksum ^= msg[a] ^ (msg[a] >> 4);

    auto button = (SomfyButton)(msg[1] >> 4);
    if((rcvChecksum & 0x0F) != checksum ||
        button != SomfyButton::Up &&
        button != SomfyButton::Down &&
        button != SomfyButton::My)
    {
        DBG_PRINT_NA("!");
        return;
    }

    uint32_t remoteId = ((uint32_t)msg[4] << 16) | ((uint32_t)msg[5] << 8) | msg[6];
    uint16_t roll = ((uint16_t)msg[2] << 8) | msg[3];
    printf("    Somfy Command received:\n    Key:    %02x\n    Btns:   %x\n    Roll:   %02x%02x\n    RemId:  %02x%02x%02x\n", msg[0], msg[1] >> 4, msg[2], msg[3], msg[4], msg[5], msg[6]);

    auto now = get_absolute_time();

    // CRITICAL SECTION
    auto save = true;
    SpinLock scopedLock(_recvLock);
    if(_recvCount)
    {
        // If this message matches the last, bump the repeat count
        if(_receivedCommands[_recvWrite].command.remoteId == remoteId &&
            _receivedCommands[_recvWrite].command.rollingCode == roll &&
            _receivedCommands[_recvWrite].command.button == button)
        {
            _receivedCommands[_recvWrite].command.repeat++;
            _receivedCommands[_recvWrite].lastMsgTime = now;
            save = false;
        }
        else
        {
            if(_recvCount == MAX_RECV_QUEUE - 1)
                // Discard
                return;
            _recvWrite = (_recvWrite + 1) % (MAX_RECV_QUEUE);
        }
    }

    if(save)
    {
        // Store this record in the circular buffer
        _receivedCommands[_recvWrite].command.remoteId = remoteId;
        _receivedCommands[_recvWrite].command.rollingCode = roll;
        _receivedCommands[_recvWrite].command.button = button;
        _receivedCommands[_recvWrite].command.repeat = 0;
        _receivedCommands[_recvWrite].lastMsgTime = now;
        _recvCount++;
    }
}