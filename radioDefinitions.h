#pragma once



#define RADIO_RegOpMode 0x01
#define RADIO_RegDataModul 0x02
#define RADIO_RegBitrate_Word 0x03
#define RADIO_RegFreqDeviation_Word 0x05
#define RADIO_RegFrequency_3Byte 0x07
#define RADIO_RegOscCalibration 0x0a
#define RADIO_RegListen 0x0d
#define RADIO_RegListenCoefIdle 0x0E
#define RADIO_RegListenCoefRx 0x0F
#define RADIO_RegVersion 0x10
#define RADIO_RegPaLevel 0x11
#define RADIO_RegPaRamp 0x12
#define RADIO_RegOcp 0x13

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

