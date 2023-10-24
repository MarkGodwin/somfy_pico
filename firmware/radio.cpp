// Copyright (c) 2023 Mark Godwin.
// SPDX-License-Identifier: MIT

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "radio.h"
#include "radioDefinitions.h"


RFM69Radio::RFM69Radio(spi_inst_t *spi, uint csPin, uint resetPin)
: _spi(spi), _csPin(csPin), _resetPin(resetPin)
{
    // Chip select is active-low, so we'll initialise it to a driven-high state
    gpio_set_function(_csPin,   GPIO_FUNC_SIO);
    gpio_set_dir(_csPin, GPIO_OUT);
    ChipSelect(false);

    gpio_init(_resetPin);
    gpio_set_dir(resetPin, GPIO_OUT);
    gpio_put(_resetPin, 0);

}

void RFM69Radio::Reset()
{
    gpio_put(_resetPin, 1);
    sleep_ms(250);
    gpio_put(_resetPin, 0);
    sleep_ms(250);
}

void RFM69Radio::Initialize()
{
    SetMode(MODE_STBY);

    DataModul dataMode;
    dataMode.data = 0;
    dataMode.dataMode = 0; // Packet mode
    dataMode.modulationType = 1; // On-off keying (OOK)
    dataMode.modulationShaping = 0;
    WriteRegister(RADIO_RegDataModul, dataMode.data);

    Ocp ocp;
    ocp.data = 0;
    ocp.on = 0;
    ocp.trim = 0xA;
    WriteRegister(RADIO_RegOcp, ocp.data); //disable OverCurrentProtection for HW/HCW...?

    PaLevel level;
    level.data = 0;
    level.outputPower = 31;
    level.pa0On = 0;
    level.pa1On = 1;
    level.pa2On = 1;
    WriteRegister(RADIO_RegPaLevel, level.data);

    SyncConfig syncConfig;
    syncConfig.syncOn = 0;
    syncConfig.errorTolerance = 0;
    syncConfig.fifoFillCondition = 0;
    syncConfig.syncSize = 7;
    WriteRegister(RADIO_RegSyncConfig, syncConfig.data);

    PacketConfig packetConfig;
    packetConfig.packetFormat = PACKET_FORMAT_FIXED;
    packetConfig.dcFree = PACKET_FORMAT_DCFREE_MANCHESTER;
    packetConfig.crcOn = 0;
    packetConfig.crcAutoClearOff = 0;
    packetConfig.addressFiltering = 0;
    packetConfig.payloadLength = 64;
    WriteRegisterWord(RADIO_RegPacketConfig, packetConfig.data);

    // No 0xAAAAAA packet preamble
    WriteRegisterWord(RADIO_RegPreambleSize_Word, 0);

}

void RFM69Radio::SetSyncBytes(const uint8_t *sync, uint8_t length)
{
    SyncConfig syncConfig;
    syncConfig.data = ReadRegister(RADIO_RegSyncConfig);
    syncConfig.syncOn = length > 0;
    syncConfig.syncSize = length - 1;
    WriteRegister(RADIO_RegSyncConfig, syncConfig.data);
    if(length > 0)
        WriteRegisterBuffer(RADIO_RegSyncValue, sync, length);
}

void RFM69Radio::SetPacketFormat(bool manchester, uint8_t payloadSize)
{
    PacketConfig packetConfig;
    packetConfig.packetFormat = payloadSize == 0 ? PACKET_FORMAT_VARIABLE : PACKET_FORMAT_FIXED;
    packetConfig.dcFree = manchester ? PACKET_FORMAT_DCFREE_MANCHESTER : PACKET_FORMAT_DCFREE_NONE;
    packetConfig.crcOn = 0;
    packetConfig.crcAutoClearOff = 0;
    packetConfig.addressFiltering = 0;
    packetConfig.payloadLength = payloadSize == 0 ? 255 : payloadSize;
    WriteRegisterWord(RADIO_RegPacketConfig, packetConfig.data);
}

void RFM69Radio::TransmitPacket(const uint8_t *buffer, size_t length)
{
    WaitForMode();
    WriteFifo(buffer, length);
    SetMode(MODE_TX);
    WaitForMode();

    WaitForPacketSent();
    SetMode(MODE_STBY);
}

void RFM69Radio::SetSymbolWidth(uint16_t us)
{
    auto br =  32 * us; // Oscilator frequency / (1,000,000 / symbol width)
    if(br > 65535)
        br = 65535;
    WriteRegisterWord(RADIO_RegBitrate_Word, (uint16_t)br);
}

void RFM69Radio::SetBitRate(uint32_t bps)
{
    auto br =  32000000 / bps; // Oscilator frequency / bitrate
    if(br > 65535)
        br = 65535;
    WriteRegisterWord(RADIO_RegBitrate_Word, (uint16_t)br);
}

uint16_t RFM69Radio::GetSymbolWidth()
{
    auto br = ReadRegisterWord(RADIO_RegBitrate_Word);
    return br / 32;
}

uint32_t RFM69Radio::GetBitRate()
{
    auto br = ReadRegisterWord(RADIO_RegBitrate_Word);
    return 32000000 / br;
}

void RFM69Radio::SetFrequency(double frequency)
{
    auto f = (uint32_t)(frequency * 1000000 / 61.03515625);
    WriteRegister3byte(RADIO_RegFrequency_3Byte, f);
}

double RFM69Radio::GetFrequency()
{
    uint32_t f = 0;
    ReadRegisterBuffer(RADIO_RegFrequency_3Byte, (uint8_t *)&f, 3);
    f = f & 0x00FF00 | ((f & 0xFF) << 16) | ((f & 0xFF0000) >> 16);     // Big-Endian reverse
    return (f * 61.03515625) / 1000000;
}

uint8_t RFM69Radio::GetVersion()
{
    return ReadRegister(RADIO_RegVersion);
}

inline void RFM69Radio::SetMode(uint8_t mode)
{
    OpMode opMode;
    opMode.sequencerOff = false;
    opMode.listenOn = false;
    opMode.listenAbort = false;
    opMode.mode = mode;
    WriteRegister(RADIO_RegOpMode, opMode.data);
}

inline bool RFM69Radio::WaitForMode()
{
    IrqFlags flags;
    for(int a = 0; a < 10000; a++)
    {
        ReadRegisterBuffer(RADIO_RegIrqFlags, (uint8_t *)&flags, sizeof(flags));
        if(flags.modeReady)
        {
            return true;
        }
        sleep_us(1);
    }

    printf("Waited too long for ready.\n");
    printf("Flags: 0x%02x%02x\n", flags.data1, flags.data2);
    return false;
}

inline bool RFM69Radio::WaitForPacketSent()
{
    IrqFlags flags;
    for(int a = 0; a < 100000; a++)
    {
        ReadRegisterBuffer(RADIO_RegIrqFlags, (uint8_t *)&flags, sizeof(flags));
        if(flags.packetSent)
        {
            return true;
        }
        sleep_us(100);
    }

    printf("Waited too long for send to complete.\n");
    printf("Flags: 0x%02x%02x\n", flags.data1, flags.data2);
    return false;
}

inline void RFM69Radio::ChipSelect(bool select)
{
    asm volatile("nop \n nop \n nop");
    gpio_put(_csPin, !select);  // Active low
    asm volatile("nop \n nop \n nop");
}


inline void RFM69Radio::WriteRegister(uint8_t reg, uint8_t data)
 {
    uint8_t buf[2];
    buf[0] = reg | 0x80;  // Set high bit for write
    buf[1] = data;
    ChipSelect(true);
    spi_write_blocking(_spi, buf, sizeof(buf));
    ChipSelect(false);
}

inline void RFM69Radio::WriteRegisterWord(uint8_t reg, uint16_t data)
 {
    uint8_t buf[3];
    buf[0] = reg | 0x80;  // Set high bit for write
    buf[1] = (data >> 8) & 0xff;
    buf[2] = data & 0xff;
    ChipSelect(true);
    spi_write_blocking(_spi, buf, sizeof(buf));
    ChipSelect(false);
}

inline void RFM69Radio::WriteRegister3byte(uint8_t reg, uint32_t data)
 {
    uint8_t buf[4];
    buf[0] = reg | 0x80;  // Set high bit for write
    buf[1] = (data >> 16) & 0xff;
    buf[2] = (data >> 8) & 0xff;
    buf[3] = data & 0xff;
    ChipSelect(true);
    spi_write_blocking(_spi, buf, sizeof(buf));
    ChipSelect(false);
}

inline void RFM69Radio::WriteRegisterBuffer(uint8_t reg, const uint8_t *buffer, uint8_t length)
{
    ChipSelect(true);
    reg |= 0x80;
    spi_write_blocking(_spi, &reg, 1);
    spi_write_blocking(_spi, buffer, length);
    ChipSelect(false);
}

inline void RFM69Radio::WriteFifo(const uint8_t *buffer, uint8_t length)
{
    WriteRegisterBuffer(0x00, buffer, length);
}

inline uint8_t RFM69Radio::ReadRegister(uint8_t reg)
 {
    ChipSelect(true);
    spi_write_blocking(_spi, &reg, 1);
    uint8_t result;
    spi_read_blocking(_spi, 0, &result, 1);
    ChipSelect(false);
    return result;
}

inline uint16_t RFM69Radio::ReadRegisterWord(uint8_t reg)
 {
    ChipSelect(true);
    spi_write_blocking(_spi, &reg, 1);
    uint16_t result;
    spi_read_blocking(_spi, 0, (uint8_t *)&result, 2);
    ChipSelect(false);
    return ((result & 0xFF) << 8) | (result >> 8);
}


inline void RFM69Radio::ReadRegisterBuffer(uint8_t reg, uint8_t *buffer, uint8_t length)
{
    ChipSelect(true);
    spi_write_blocking(_spi, &reg, 1);
    uint8_t result;
    spi_read_blocking(_spi, 0, buffer, length);
    ChipSelect(false);
}

inline void RFM69Radio::ReadFifo(uint8_t *buffer, uint8_t length)
{
    ChipSelect(true);
    uint8_t address = 0;
    spi_write_blocking(_spi, &address, 1);
    spi_read_blocking(_spi, 0, buffer, length);
    ChipSelect(false);
}
