#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/flash.h"
#include <string.h>
#include "commandQueue.h"
#include "radio.h"

struct CommandEntry
{
    uint32_t commandType;
    uint32_t remoteId;
    uint16_t rollingCode;
    uint16_t repeat;
    SomfyButton button;
};

RadioCommandQueue::RadioCommandQueue(std::shared_ptr<RFM69Radio> radio)
:   _radio(std::move(radio))
{
    queue_init(&_queue, sizeof(CommandEntry), 16);
}

bool RadioCommandQueue::QueueCommand(uint32_t remoteId, uint16_t rollingCode, SomfyButton button, uint16_t repeat)
{
    CommandEntry entry = { 
        commandType: 1,
        remoteId: remoteId,
        rollingCode: rollingCode,
        repeat: repeat,
        button: button
    };

    return queue_try_add(&_queue, &entry);
}

void RadioCommandQueue::Shutdown()
{
    CommandEntry entry = { 
        commandType: 0
    };

    queue_add_blocking(&_queue, &entry);

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
            printf("Flash init failed\n");
            return;
        }

        _thequeue->Worker();
        flash_safe_execute_core_deinit();
    });
}

void RadioCommandQueue::Worker()
{
    _radio->Reset();
    puts("Radio Reset complete!\n");
    _radio->Initialize();
    _radio->SetSymbolWidth(635);
    _radio->SetFrequency(433.44);
    auto fq = _radio->GetFrequency();
    printf("Radio frequency: %.3f\n", fq);
    auto br = _radio->GetBitRate();
    printf("BitRate: %dbps\n", br);
    auto ver = _radio->GetVersion();
    printf("Radio version: 0x%02x\n", ver);

    while(true)
    {
        CommandEntry entry;
        queue_remove_blocking(&_queue, &entry);

        switch(entry.commandType)
        {
            case 0:
                return;
            case 1:
                ExecuteCommand(entry.remoteId, entry.rollingCode, entry.button, entry.repeat);
        }
    }
}

void RadioCommandQueue::ExecuteCommand(uint32_t remoteId, uint16_t rollingCode, SomfyButton button, uint16_t repeat)
{
    _radio->SetSyncBytes(NULL, 0);
    _radio->SetPacketFormat(false, 2);
    auto now = get_absolute_time();

    // We have to put 16 bytes into the fifo as a minumum for reasons I cannot explain.
    // Otherwise the radio hangs
    uint8_t fog[16];
    ::memset(fog, 0xFF, 16);
    _radio->TransmitPacket(fog, 16);

    uint8_t payload[16] = {
        0xA7,                       // "Key". Fixed...
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

    _radio->TransmitPacket(payload, sizeof(payload));
    _radio->SetSyncBytes(syncBytes, 8);
    packetStartTime = delayed_by_us(packetStartTime, 115000);

    for(auto a = 0; a < repeat; a++)
    {
        while(absolute_time_diff_us(get_absolute_time(), packetStartTime) > 0);

        _radio->TransmitPacket(payload, sizeof(payload));
        packetStartTime = delayed_by_us(packetStartTime, 139000);
    }
}