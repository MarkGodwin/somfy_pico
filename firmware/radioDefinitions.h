// Copyright (c) 2023 Mark Godwin.
// SPDX-License-Identifier: MIT

// Definitions for the RFM69HCW module SPI API for Pi Pico architecture

#pragma once

#define RADIO_RegOpMode 0x01
#define RADIO_RegDataModul 0x02
#define RADIO_RegBitrate_Word 0x03
#define RADIO_RegFreqDeviation_Word 0x05
#define RADIO_RegFrequency_3Byte 0x07
#define RADIO_RegOscCalibration 0x0a
#define RADIO_RegListen 0x0d
#define RADIO_RegVersion 0x10
#define RADIO_RegPaLevel 0x11
#define RADIO_RegPaRamp 0x12
#define RADIO_RegOcp 0x13
#define RADIO_RegLna 0x18
#define RADIO_RegRxBw 0x19

#define RADIO_RegOokPeak 0x1b
#define RADIO_RegOokAvg  0x1c
#define RADIO_RegOokFix  0x1d

#define RADIO_RegDioMapping 0x25
#define RADIO_RegIrqFlags 0x27
#define RADIO_RegRssiThreshold 0x29
#define RADIO_RegRxTimeoutRxStart 0x2a
#define RADIO_RegRxTimeoutRssiThresh 0x2b

#define RADIO_RegPreambleSize_Word 0x2c
#define RADIO_RegSyncConfig 0x2e
#define RADIO_RegSyncValue 0x2f
#define RADIO_RegPacketConfig 0x37
#define RADIO_RegPayloadLength 0x38
#define RADIO_RegNodeAddress 0x39
#define RADIO_RegBroadcastAddress 0x3a
#define RADIO_RegAutoModes 0x3b
#define RADIO_RegFifoThreshold 0x3c
#define RADIO_RegPacketConfig2 0x3d
#define RADIO_RegAesKey 0x3e


union OpMode
{
    uint8_t data;
    struct {
        uint8_t unused : 2;
        uint8_t mode :3;
        uint8_t listenAbort : 1;
        uint8_t listenOn : 1;
        uint8_t sequencerOff : 1;
    };
};

union DataModul
{
    uint8_t data;
    struct {
        uint8_t modulationShaping : 2;
        uint8_t unused : 1;
        uint8_t modulationType : 2;
        uint8_t dataMode : 2;
        uint8_t unused2 :1;
    };
};

#define MODE_SLEEP 0
#define MODE_STBY 1
#define MODE_FS 2
#define MODE_TX 3
#define MODE_RX 4

union RadioVersion
{
    uint8_t data;
    struct {
        uint8_t revision:4;
        uint8_t version:4;
    };
};

union PaLevel
{
    uint8_t data;
    struct {
        uint8_t outputPower:5;
        uint8_t pa2On:1;
        uint8_t pa1On:1;
        uint8_t pa0On:1;
    };
};

union Ocp
{
    uint8_t data;
    struct {
        uint8_t trim:4;
        uint8_t on:1;
        uint8_t unused:3;
    };
};

#define LNA_IMPEDANCE_50  0
#define LNA_IMPEDANCE_200 1

#define LNA_GAIN_AUTO  0x00
#define LNA_GAIN_MAX   0x01
#define LNA_GAIN_6DB   0x02
#define LNA_GAIN_12DB  0x03
#define LNA_GAIN_24DB  0x04
#define LNA_GAIN_36DB  0x05
#define LNA_GAIN_48DB  0x06

union Lna
{
    uint8_t data;
    struct {
        uint8_t gainSelect:3;
        uint8_t currentGain:3;
        uint8_t unused:1;
        uint8_t impedance:1;
    };
};


#define BW_MANT_16 0x00
#define BW_MANT_20 0x01
#define BW_MANT_24 0x02

union RxBw
{
    uint8_t data;
    struct {
        uint8_t bwExp : 3;
        uint8_t bwMant : 2;
        uint8_t freq : 3;
    };
};

union OokPeak
{
    uint8_t data;
    struct {
        uint8_t threshDec : 3;
        uint8_t threshStep : 3;
        uint8_t threshType : 2;
    };
};

union OokAvg
{
    uint8_t data;
    struct {
        uint8_t unused : 6;
        uint8_t threshFilt : 2;
    };
};

#define CLKOUT_OSC 0x00
#define CLKOUT_RC  0x06
#define CLKOUT_OFF 0x07
#define DIOMAPPING_DEFAULT 0x00

union DioMapping
{
    uint16_t data;
    struct {
        uint16_t clkOut : 3;
        uint16_t unused : 1;
        uint16_t dio5Mapping : 2;
        uint16_t dio4Mapping : 2;
        uint16_t dio3Mapping : 2;
        uint16_t dio2Mapping : 2;
        uint16_t dio1Mapping : 2;
        uint16_t dio0Mapping : 2;
    };
    
};

struct IrqFlags
{
    union {
        uint8_t data1;
        struct {
            uint8_t syncAddressMatch:1;
            uint8_t autoMode:1;
            uint8_t timeout:1;
            uint8_t rssi:1;
            uint8_t pllLock:1;
            uint8_t txReady:1;
            uint8_t rxReady:1;
            uint8_t modeReady:1;
        };
    };
    union {
        uint8_t data2;
        struct {
            uint8_t unused:1;
            uint8_t crcOk:1;
            uint8_t payloadReady:1;
            uint8_t packetSent:1;
            uint8_t fifoOverrun:1;
            uint8_t fifoLevel:1;
            uint8_t fifoNotEmpty:1;
            uint8_t fifoFull:1;
        };
    };
};

union SyncConfig
{
    uint8_t data;
    struct {
        uint8_t errorTolerance:3;
        uint8_t syncSize:3;
        uint8_t fifoFillCondition:1;
        uint8_t syncOn:1;
    };
};

union PacketConfig
{
    uint16_t data;
    struct {
        uint16_t payloadLength:8;
        uint16_t unused:1;
        uint16_t addressFiltering:2;
        uint16_t crcAutoClearOff:1;
        uint16_t crcOn:1;
        uint16_t dcFree:2;
        uint16_t packetFormat:1;
    };
};

union PacketConfig2
{
    uint8_t data;
    struct {
        uint8_t aesOn:1;
        uint8_t autoRxRestartOn:1;
        uint8_t restartRx:1;
        uint8_t unused:1;
        uint8_t interPacketRxDelay:4;
    };
};


#define PACKET_FORMAT_FIXED 0
#define PACKET_FORMAT_VARIABLE 1
#define PACKET_FORMAT_DCFREE_NONE 0
#define PACKET_FORMAT_DCFREE_MANCHESTER 1
#define PACKET_FORMAT_DCFREE_WHITENING 2

union AutoModes
{
    uint8_t data;
    struct {
        uint8_t intermediateMode:2;
        uint8_t exitCondition:3;
        uint8_t enterCondition:3;
    };
};

union FifoThreshold
{
    uint8_t data;
    struct {
        uint8_t fifoThreshold:7;
        uint8_t txStartCondition:1;
    };
};

#define LISTEN_COEF_IDLE_DEFAULT 0xf5
#define LISTEN_COEF_RX_DEFAULT 20

#define LISTEN_RESOL_64US 0x01
#define LISTEN_RESOL_4MS 0x02
#define LISTEN_RESOL_262MS 0x03

#define LISTEN_CRITERIA_THRESHOLD 0x00
#define LISTEN_CRITERIA_SYNC 0x01

#define LISTEN_END_RX     0x00
#define LISTEN_END_MODE   0x01
#define LISTEN_END_RESUME 0x02

union ListenMode
{
    uint32_t data;  // 3 BYTES ONLY!
    struct {
        uint32_t unused1 : 8;
        uint8_t listenCoefRx : 8;
        uint8_t listenCoefIdle : 8;
        uint32_t unused2 : 1;
        uint32_t listenEnd : 2;
        uint32_t listenCriteria : 1;
        uint32_t listenResolRx : 2;
        uint32_t listenResolIdle : 2;
    };
};