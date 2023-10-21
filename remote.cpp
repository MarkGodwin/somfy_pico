#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "pico.h"
#include "pico/stdlib.h"
#include "pico/mem_ops.h"
#include "hardware/spi.h"
#include "radio.h"
#include "remote.h"

SomfyRemote::SomfyRemote(RFM69Radio *radio, uint32_t remoteId, uint16_t rollingCode)
: _radio(radio), _remoteId(remoteId), _rollingCode(rollingCode)
{

}

void SomfyRemote::PressButtons(SomfyButton buttons, uint16_t repeat)
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
        0xA7,                       // Key. Fixed...
        (uint8_t)(buttons << 4),
        (uint8_t)(_rollingCode >> 8),
        (uint8_t)(_rollingCode & 0xFF),
        (uint8_t)((_remoteId >> 16) & 0xFF),
        (uint8_t)((_remoteId >> 8) & 0xFF),
        (uint8_t)(_remoteId & 0xFF),
    };            
    memset(payload + 7, 0, 9);
    _rollingCode++;

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