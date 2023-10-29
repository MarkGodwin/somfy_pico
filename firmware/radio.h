// Copyright (c) 2023 Mark Godwin.
// SPDX-License-Identifier: MIT

#pragma once

#include "hardware/spi.h"


class RFM69Radio
{
public:
    RFM69Radio(spi_inst_t *spi, uint csPin, uint resetPin, uint packetPin);

    void SetSymbolWidth(uint16_t us);
    void SetBitRate(uint32_t bps);
    uint16_t GetSymbolWidth();
    uint32_t GetBitRate();

    // Set frequency in MHz
    void SetFrequency(double frequency);
    double GetFrequency();
    uint8_t GetVersion();

    void Reset();
    void Initialize();

    void SetSyncBytes(const uint8_t *sync, uint8_t length);
    void SetPacketFormat(bool manchester, uint8_t payloadSize);

    void TransmitPacket(const uint8_t *buffer, size_t length);
    void ReceivePacket(uint8_t *buffer, size_t length);
    void EnableReceive(void (*cb)());
    void Standby();

private:

    void SetMode(uint8_t mode, bool listen = false);
    bool WaitForMode();
    bool WaitForPacketSent();

    void ChipSelect(bool select);
    void WriteRegisterBuffer(uint8_t reg, const uint8_t *buffer, uint8_t length);
    void WriteRegister(uint8_t reg, uint8_t data);
    void WriteRegisterWord(uint8_t reg, uint16_t data);
    void WriteRegister3byte(uint8_t reg, uint32_t data);
    void WriteFifo(const uint8_t *buffer, uint8_t length);
    uint8_t ReadRegister(uint8_t reg);
    uint16_t ReadRegisterWord(uint8_t reg);
    void ReadRegisterBuffer(uint8_t reg, uint8_t *buffer, uint8_t length);
    void ReadFifo(uint8_t *buffer, uint8_t length);

    bool _listen;
    spi_inst_t *_spi;
    uint _csPin;
    uint _resetPin;
    uint _packetPin;

};