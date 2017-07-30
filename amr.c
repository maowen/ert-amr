#include "amr.h"
#include <string.h>

#ifdef PLATFORM_ESP8266
#include "platform/esp8266/amr_hal.h"
#include <osapi.h>
#else
#include <stdio.h>
#endif


#define RX_BUF_SIZE (AMR_MSG_IDM_RAW_SIZE + sizeof(AmrMsgHeader))
#define PROC_RING_BUF_SIZE 512

static uint8_t rxBuf0[RX_BUF_SIZE] = {0}; //! Manchester decoded data buffer 0
static uint8_t rxBuf1[RX_BUF_SIZE] = {0}; //! Manchester decoded data buffer 1
static uint8_t *rxBuf = rxBuf0; //! Active data bufer

static uint8_t prevRxBit = 0;
static AmrScmMsg scmMsg = {0};
static AmrScmPlusMsg scmPlusMsg = {0};
static AmrIdmMsg idmMsg = {0};

static void (*scmMsgCallback)(const AmrScmMsg * msg) = NULL;
static void (*scmPlusMsgCallback)(const AmrScmPlusMsg * msg) = NULL;
static void (*idmMsgCallback)(const AmrIdmMsg * msg) = NULL;

static Ring msgRing;
static uint8_t msgRingData[PROC_RING_BUF_SIZE] = {0};

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define NTOH_16BIT(num) ((((uint16_t)(num) << 8) & 0xff00) | (((uint16_t)(num) >> 8) & 0xff))
#define NTOH_32BIT(num) \
    ((((uint32_t)(num)>>24)&0xff) | (((uint32_t)(num)<<8)&0xff0000) | \
    (((uint32_t)(num)>>8)&0xff00) | (((uint32_t)(num)<<24)&0xff000000))
#define HTON_16BIT(num) ((((uint16_t)(num) << 8) & 0xff00) | (((uint16_t)(num) >> 8) & 0xff))
#define HTON_32BIT(num) \
    ((((uint32_t)(num)>>24)&0xff) | (((uint32_t)(num)<<8)&0xff0000) | \
    (((uint32_t)(num)>>8)&0xff00) | (((uint32_t)(num)<<24)&0xff000000))
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define NTOH_16BIT(num) (num)
#define NTOH_32BIT(num) (num)
#define HTON_16BIT(num) (num)
#define HTON_32BIT(num) (num)
#else
#error Only __ORDER_LITTLE_ENDIAN__ and __ORDER_BIG_ENDIAN__ are supported.
#endif

/* BCH CRC-16 Table Generation Code
void calcBCHCRCTable() {
    const uint16_t poly = 0x6f63;

    printf("const uint16_t crc16Table[256] = {");
    for (size_t div = 0; div < sizeof(crc16Table)/sizeof(crc16Table[0]); div++) {
        uint16_t curByte = (uint16_t)(div) << 8;
        for (uint8_t bit = 0; bit < 8; bit++) {
            if ((curByte & 0x8000) != 0) {
                curByte = (curByte << 1) ^ poly;
            }
            else {
                curByte = curByte << 1;
            }
        }
        printf("0x%04x,", curByte);
        crc16Table[div] = curByte;
    }
    printf("}");
}
*/

/* BCH CRC-16 Lookup table */
static const uint16_t crc16BCHTable[256] = {
    0x0000,0x6f63,0xdec6,0xb1a5,0xd2ef,0xbd8c,0x0c29,0x634a,0xcabd,0xa5de,
    0x147b,0x7b18,0x1852,0x7731,0xc694,0xa9f7,0xfa19,0x957a,0x24df,0x4bbc,
    0x28f6,0x4795,0xf630,0x9953,0x30a4,0x5fc7,0xee62,0x8101,0xe24b,0x8d28,
    0x3c8d,0x53ee,0x9b51,0xf432,0x4597,0x2af4,0x49be,0x26dd,0x9778,0xf81b,
    0x51ec,0x3e8f,0x8f2a,0xe049,0x8303,0xec60,0x5dc5,0x32a6,0x6148,0x0e2b,
    0xbf8e,0xd0ed,0xb3a7,0xdcc4,0x6d61,0x0202,0xabf5,0xc496,0x7533,0x1a50,
    0x791a,0x1679,0xa7dc,0xc8bf,0x59c1,0x36a2,0x8707,0xe864,0x8b2e,0xe44d,
    0x55e8,0x3a8b,0x937c,0xfc1f,0x4dba,0x22d9,0x4193,0x2ef0,0x9f55,0xf036,
    0xa3d8,0xccbb,0x7d1e,0x127d,0x7137,0x1e54,0xaff1,0xc092,0x6965,0x0606,
    0xb7a3,0xd8c0,0xbb8a,0xd4e9,0x654c,0x0a2f,0xc290,0xadf3,0x1c56,0x7335,
    0x107f,0x7f1c,0xceb9,0xa1da,0x082d,0x674e,0xd6eb,0xb988,0xdac2,0xb5a1,
    0x0404,0x6b67,0x3889,0x57ea,0xe64f,0x892c,0xea66,0x8505,0x34a0,0x5bc3,
    0xf234,0x9d57,0x2cf2,0x4391,0x20db,0x4fb8,0xfe1d,0x917e,0xb382,0xdce1,
    0x6d44,0x0227,0x616d,0x0e0e,0xbfab,0xd0c8,0x793f,0x165c,0xa7f9,0xc89a,
    0xabd0,0xc4b3,0x7516,0x1a75,0x499b,0x26f8,0x975d,0xf83e,0x9b74,0xf417,
    0x45b2,0x2ad1,0x8326,0xec45,0x5de0,0x3283,0x51c9,0x3eaa,0x8f0f,0xe06c,
    0x28d3,0x47b0,0xf615,0x9976,0xfa3c,0x955f,0x24fa,0x4b99,0xe26e,0x8d0d,
    0x3ca8,0x53cb,0x3081,0x5fe2,0xee47,0x8124,0xd2ca,0xbda9,0x0c0c,0x636f,
    0x0025,0x6f46,0xdee3,0xb180,0x1877,0x7714,0xc6b1,0xa9d2,0xca98,0xa5fb,
    0x145e,0x7b3d,0xea43,0x8520,0x3485,0x5be6,0x38ac,0x57cf,0xe66a,0x8909,
    0x20fe,0x4f9d,0xfe38,0x915b,0xf211,0x9d72,0x2cd7,0x43b4,0x105a,0x7f39,
    0xce9c,0xa1ff,0xc2b5,0xadd6,0x1c73,0x7310,0xdae7,0xb584,0x0421,0x6b42,
    0x0808,0x676b,0xd6ce,0xb9ad,0x7112,0x1e71,0xafd4,0xc0b7,0xa3fd,0xcc9e,
    0x7d3b,0x1258,0xbbaf,0xd4cc,0x6569,0x0a0a,0x6940,0x0623,0xb786,0xd8e5,
    0x8b0b,0xe468,0x55cd,0x3aae,0x59e4,0x3687,0x8722,0xe841,0x41b6,0x2ed5,
    0x9f70,0xf013,0x9359,0xfc3a,0x4d9f,0x22fc};

// Evaluate BCH CRC-16
static inline uint8_t computeBCHCRC(const uint8_t * data, size_t len) {
    uint16_t crc = 0;
    uint16_t i = 0;
    for (; i < len; i++) {
        crc = (crc << 8) ^ crc16BCHTable[(crc >> 8) ^ data[i]];
    }

    return crc == 0x0000; /* Compare to residual value */
}

/* ccitt crc-16 table generation code
void calcccittcrctable() {
    const uint16_t poly = 0x1021;

    printf("const uint16_t crc16ccitttable[256] = {");
    for (size_t div = 0; div < sizeof(crc16table)/sizeof(crc16table[0]); div++) {
        uint16_t curbyte = (uint16_t)(div) << 8;
        for (uint8_t bit = 0; bit < 8; bit++) {
            if ((curbyte & 0x8000) != 0) {
                curbyte = (curbyte << 1) ^ poly;
            }
            else {
                curbyte = curbyte << 1;
            }
        }
        printf("0x%04x,", curbyte);
    }
    printf("}");
}
*/

/* CCITT CRC-16 Lookup table */
static const uint16_t crc16CCITTTable[256] = {
    0x0000,0x1021,0x2042,0x3063,0x4084,0x50a5,0x60c6,0x70e7,0x8108,0x9129,
    0xa14a,0xb16b,0xc18c,0xd1ad,0xe1ce,0xf1ef,0x1231,0x0210,0x3273,0x2252,
    0x52b5,0x4294,0x72f7,0x62d6,0x9339,0x8318,0xb37b,0xa35a,0xd3bd,0xc39c,
    0xf3ff,0xe3de,0x2462,0x3443,0x0420,0x1401,0x64e6,0x74c7,0x44a4,0x5485,
    0xa56a,0xb54b,0x8528,0x9509,0xe5ee,0xf5cf,0xc5ac,0xd58d,0x3653,0x2672,
    0x1611,0x0630,0x76d7,0x66f6,0x5695,0x46b4,0xb75b,0xa77a,0x9719,0x8738,
    0xf7df,0xe7fe,0xd79d,0xc7bc,0x48c4,0x58e5,0x6886,0x78a7,0x0840,0x1861,
    0x2802,0x3823,0xc9cc,0xd9ed,0xe98e,0xf9af,0x8948,0x9969,0xa90a,0xb92b,
    0x5af5,0x4ad4,0x7ab7,0x6a96,0x1a71,0x0a50,0x3a33,0x2a12,0xdbfd,0xcbdc,
    0xfbbf,0xeb9e,0x9b79,0x8b58,0xbb3b,0xab1a,0x6ca6,0x7c87,0x4ce4,0x5cc5,
    0x2c22,0x3c03,0x0c60,0x1c41,0xedae,0xfd8f,0xcdec,0xddcd,0xad2a,0xbd0b,
    0x8d68,0x9d49,0x7e97,0x6eb6,0x5ed5,0x4ef4,0x3e13,0x2e32,0x1e51,0x0e70,
    0xff9f,0xefbe,0xdfdd,0xcffc,0xbf1b,0xaf3a,0x9f59,0x8f78,0x9188,0x81a9,
    0xb1ca,0xa1eb,0xd10c,0xc12d,0xf14e,0xe16f,0x1080,0x00a1,0x30c2,0x20e3,
    0x5004,0x4025,0x7046,0x6067,0x83b9,0x9398,0xa3fb,0xb3da,0xc33d,0xd31c,
    0xe37f,0xf35e,0x02b1,0x1290,0x22f3,0x32d2,0x4235,0x5214,0x6277,0x7256,
    0xb5ea,0xa5cb,0x95a8,0x8589,0xf56e,0xe54f,0xd52c,0xc50d,0x34e2,0x24c3,
    0x14a0,0x0481,0x7466,0x6447,0x5424,0x4405,0xa7db,0xb7fa,0x8799,0x97b8,
    0xe75f,0xf77e,0xc71d,0xd73c,0x26d3,0x36f2,0x0691,0x16b0,0x6657,0x7676,
    0x4615,0x5634,0xd94c,0xc96d,0xf90e,0xe92f,0x99c8,0x89e9,0xb98a,0xa9ab,
    0x5844,0x4865,0x7806,0x6827,0x18c0,0x08e1,0x3882,0x28a3,0xcb7d,0xdb5c,
    0xeb3f,0xfb1e,0x8bf9,0x9bd8,0xabbb,0xbb9a,0x4a75,0x5a54,0x6a37,0x7a16,
    0x0af1,0x1ad0,0x2ab3,0x3a92,0xfd2e,0xed0f,0xdd6c,0xcd4d,0xbdaa,0xad8b,
    0x9de8,0x8dc9,0x7c26,0x6c07,0x5c64,0x4c45,0x3ca2,0x2c83,0x1ce0,0x0cc1,
    0xef1f,0xff3e,0xcf5d,0xdf7c,0xaf9b,0xbfba,0x8fd9,0x9ff8,0x6e17,0x7e36,
    0x4e55,0x5e74,0x2e93,0x3eb2,0x0ed1,0x1ef0};

// Evaluate CCITT CRC-16
static inline uint16_t computeCCITTCRC(const uint8_t * data, size_t len) {
    uint16_t crc = 0xffff; /* init value */
    uint16_t i = 0;
    for (; i < len; i++) {
        crc = (crc << 8) ^ crc16CCITTTable[(crc >> 8) ^ data[i]];
    }

    return crc == 0x1D0F; /* Compare to residual value */
}
void amrInit() {
    msgRing = ringInit(msgRingData, sizeof(msgRingData));
    amrHalInit();
}

void amrProcessRxBit(uint8_t rxBit) {
    // The decoding of the current bit depends on the previous bit and the
    // result is stored in alternating buffers because there are two possible
    // alignments of the encoded data.

    // Select active buffer of manchester decoded data
    rxBuf = (rxBuf != rxBuf0) ? rxBuf0 : rxBuf1;

    // Manchester decode the current bit based on the previous bit
    uint8_t manchBit = (prevRxBit && !rxBit) ? 0x1 : 0x0;

    // Shift manchester decoded buffer contents and append current decoded bit
    uint8_t i = 0;
    for(; i < RX_BUF_SIZE-1; i++ ) {
        rxBuf[i] = (rxBuf[i] << 1) | (rxBuf[i+1] >> 7);
    }
    rxBuf[RX_BUF_SIZE-1] = (rxBuf[RX_BUF_SIZE-1] << 1) | manchBit;

    prevRxBit = rxBit;

    // Start looking for the message preambled/sync at the first location in
    // the buffer that it will fit. (This minimizes any buffering, processing
    // delay)
    // Leave space at the front of the Rx buffer to populate the message header
    // before pushing it onto the processing ring buffer
    uint8_t* scmData = rxBuf + RX_BUF_SIZE - AMR_MSG_SCM_RAW_SIZE;
    uint8_t* scmPlusData = rxBuf + RX_BUF_SIZE - AMR_MSG_SCM_PLUS_RAW_SIZE;
    uint8_t* idmData = rxBuf + RX_BUF_SIZE - AMR_MSG_IDM_RAW_SIZE;

    // Check for 21-bit SCM preamble/sync match (0x1F2A60) in first 21 of 24 bits
    if (scmData[0] == 0xf9 &&
            scmData[1] == 0x53 &&
            (scmData[2] & 0xf8) == 0x00) {
        AmrMsgHeader * hdr = (AmrMsgHeader *)(scmData - sizeof(AmrMsgHeader));
        hdr->type = AMR_MSG_TYPE_SCM;
        // TODO platform agnostic time
        /* hdr->timestamp = system_get_time() / 1000; // Convert micro to millis */
        RING_STATUS status =
            ringPush(&msgRing, (uint8_t*)hdr,
                    AMR_MSG_SCM_RAW_SIZE + sizeof(AmrMsgHeader));
        if (status != RING_STATUS_OK) {
            printf("Failed to push msg onto ring. Status: %u\n", status);
        }
    }
    else if (scmPlusData[0] == 0x16 &&
            scmPlusData[1] == 0xa3) {
        AmrMsgHeader * hdr = (AmrMsgHeader *)(scmPlusData - sizeof(AmrMsgHeader));
        hdr->type = AMR_MSG_TYPE_SCM_PLUS;
        // TODO platform agnostic time
        /* hdr->timestamp = system_get_time() / 1000; // Convert micro to millis */
        RING_STATUS status =
            ringPush(&msgRing, (uint8_t*)hdr,
                    AMR_MSG_SCM_PLUS_RAW_SIZE + sizeof(AmrMsgHeader));
        if (status != RING_STATUS_OK) {
            printf("Failed to push msg onto ring. Status: %u\n", status);
        }
    }
    // Check for 32-bit IDM preamble/sync match (0x555516A3)
    else if (idmData[0] == 0x55 && idmData[1] == 0x55 &&
            idmData[2] == 0x16 && idmData[3] == 0xA3) {
        AmrMsgHeader * hdr = (AmrMsgHeader *)(idmData - sizeof(AmrMsgHeader));
        hdr->type = AMR_MSG_TYPE_IDM;
        // TODO platform agnostic time
        /* hdr->timestamp = system_get_time() / 1000; // Convert micro to millis */
        RING_STATUS status =
            ringPush(&msgRing, (uint8_t*)hdr,
                    AMR_MSG_IDM_RAW_SIZE + sizeof(AmrMsgHeader));
        if (status != RING_STATUS_OK) {
            printf("Failed to push msg onto ring. Status: %d\n", status);
        }
    }
}

static inline void parseSCMMsg(const uint8_t *data, uint32_t t_ms) {
    if(computeBCHCRC(data+2, 10)) {
        scmMsg.id =
            ((data[2] & 0x6) << 23) |
            (data[7] << 16) |
            (data[8] << 8) |
            data[9];
        scmMsg.consumption =
            data[4] << 16 |
            data[5] << 8 |
            data[6];
        scmMsg.type = (data[3] >> 2) & 0xf;
        scmMsg.tamper_phy = (data[3] >> 6) & 0x3;
        scmMsg.tamper_enc = data[3] & 0x3;
        scmMsg.crc = data[10] << 8 | data[11];

        if (scmMsgCallback) {
            scmMsgCallback(&scmMsg);
        }
    }
    else {
        debug_printf("INAVLID SCM CHECKSUM\n");
    }
}

static inline void parseSCMPlusMsg(const uint8_t *data, uint32_t t_ms) {
    if(computeCCITTCRC(data+2, sizeof(AmrScmPlusMsg)-2)) {
        memcpy(&scmPlusMsg, data, sizeof(AmrScmPlusMsg));
        scmPlusMsg.frameSync = NTOH_16BIT(scmPlusMsg.frameSync);
        scmPlusMsg.protocolId = scmPlusMsg.protocolId; // No op
        scmPlusMsg.endpointType = scmPlusMsg.endpointType; // No op
        scmPlusMsg.endpointId = NTOH_32BIT(scmPlusMsg.endpointId);
        scmPlusMsg.consumption = NTOH_32BIT(scmPlusMsg.consumption);
        scmPlusMsg.tamper = NTOH_16BIT(scmPlusMsg.tamper);
        scmPlusMsg.crc = NTOH_16BIT(scmPlusMsg.crc);

        if (scmMsgCallback) {
            scmPlusMsgCallback(&scmPlusMsg);
        }
    }
    else {
        debug_printf("INAVLID SCM+ CHECKSUM\n");
    }
}

static inline void parseIDMMsg(const uint8_t *data, uint32_t t_ms) {
    if (computeCCITTCRC(data+4, 88)) {
        memcpy(&idmMsg, data, 33);
        data+=33;

        idmMsg.preamble = NTOH_32BIT(idmMsg.preamble);
        idmMsg.ertType &= 0x0F;
        idmMsg.ertId = NTOH_32BIT(idmMsg.ertId);
        // idmMsg.tamperCounters; // No op
        idmMsg.asyncCnt = NTOH_16BIT(idmMsg.asyncCnt);
        // idm.powerOutageFlags; // No op
        idmMsg.lastConsumption = NTOH_32BIT(idmMsg.lastConsumption);

        uint8_t cnt = 0;
        for (; cnt < 47; cnt++) {
            uint16_t bit = cnt * 9; /* Absolute bit */
            uint8_t i = bit / 8; /* Starting byte */
            uint8_t r = bit % 8; /* Starting bit within starting byte */
            uint8_t fm = (0xff >> r); /* Front byte bitmask */
            uint8_t bm = ~((0x80 >> r) - 1); /* Back byte bitmask */
            idmMsg.differentialConsumption[cnt] =
                ((uint16_t)(data[i] & fm) << (r+1)) | ((data[i+1] & bm) >> (7-r));
        }
        data+=53;

        memcpy(&(idmMsg.txTimeOffset), data, 6);
        data+=6;

        idmMsg.txTimeOffset = NTOH_16BIT(idmMsg.txTimeOffset);
        idmMsg.serialNumberCRC = NTOH_16BIT(idmMsg.serialNumberCRC);
        idmMsg.pktCRC = NTOH_16BIT(idmMsg.pktCRC);

        if (idmMsgCallback) {
            idmMsgCallback(&idmMsg);
        }
    }
    else {
        debug_printf("INVALID IDM CHECKSUM\n");
    }
}

void amrProcessMsgs() {
    while (true) {
        uint8_t * peek = 0;
        RingPos_t size = ringPeek(&msgRing, &peek); 
        // Message available
        if (size > 0 && peek) {
            AmrMsgHeader* hdr = (AmrMsgHeader*)peek;
            uint8_t* msgData = peek + sizeof(AmrMsgHeader);

            switch (hdr->type) {
                case AMR_MSG_TYPE_SCM:
                    {
                        parseSCMMsg(msgData, hdr->timestamp);
                    }
                    break;
                case AMR_MSG_TYPE_SCM_PLUS:
                    {
                        parseSCMPlusMsg(msgData, hdr->timestamp);
                    }
                    break;
                case AMR_MSG_TYPE_IDM:
                    {
                        parseIDMMsg(msgData, hdr->timestamp);
                    }
                    break;
                default:
                    {
                        debug_printf("Unhandled message type: %u\n", hdr->type);
                    }
                    break;
            }
            ringPop(&msgRing, NULL, 0);
        }
        // No message ready
        else {
            break;
        }
    }
}

void printIdmMsg(const AmrIdmMsg * msg) {
    printf(
            "{Time:YYYY-MM-DDTHH:MM:SS.MMM IDM:{Preamble:0x%08X PacketTypeId:0x%02X PacketLength:0x%02X "
            "HammingCode:0x%02X ApplicationVersion:0x%02X ERTType:0x%02X "
            "ERTSerialNumber:%10u ConsumptionIntervalCount:%u "
            "ModuleProgrammingState:0x%02X TamperCounters:%02X%02X%02X%02X%02X%02X "
            "AsynchronousCounters:0x%02X PowerOutageFlags:%02X%02X%02X%02X%02X%02X "
            "LastConsumptionCount:%u DifferentialConsumptionIntervals:[",
            msg->preamble, msg->packetTypeID, msg->packetLength,
            msg->hammingCode, msg->appVersion, msg->ertType, msg->ertId,
            msg->consumptionIntervalCount, msg->moduleProgrammingState,
            msg->tamperCounters[0], msg->tamperCounters[1],
            msg->tamperCounters[2], msg->tamperCounters[3],
            msg->tamperCounters[4], msg->tamperCounters[5], msg->asyncCnt,
            msg->powerOutageFlags[0], msg->powerOutageFlags[1],
            msg->powerOutageFlags[2], msg->powerOutageFlags[3],
            msg->powerOutageFlags[4], msg->powerOutageFlags[5],
            msg->lastConsumption);
    printf("%u", msg->differentialConsumption[0]);
    uint8_t i = 1;
    while (i < 47) {
        printf(" %u", msg->differentialConsumption[i++]);
    }
    printf("] TransmitTimeOffset:%u SerialNumberCRC:0x%04X PacketCRC:0x%02X}}\n",
            msg->txTimeOffset, msg->serialNumberCRC, msg->pktCRC);
}

void printScmMsg(const AmrScmMsg * msg) {
    printf(
            "{Time:YYYY-MM-DDTHH:MM:SS.MMM SCM:{ID:%u Type: %u Tamper:{Phy:%02u Enc:%02u} "
            "Consumption: %7u CRC:0x%04x}}\n",
            msg->id, msg->type, msg->tamper_phy,
            msg->tamper_enc, msg->consumption,
            msg->crc);
}

void printScmPlusMsg(const AmrScmPlusMsg * msg) {
    printf(
            "{Time:YYYY-MM-DDTHH:MM:SS.MMM SCM+:{ProtocolId:0x%02X "
            "EndpointType:0x%02X EndpointID:%10u Consumption:%10u "
            " Tamper:0x%04X PacketCRC:0x%04x}}\n",
            msg->protocolId, msg->endpointType, msg->endpointId,
            msg->consumption, msg->tamper, msg->crc);
}

void registerScmMsgCallback(void (*callback)(const AmrScmMsg * msg)) {
    scmMsgCallback = callback;
}

void registerScmPlusMsgCallback(void (*callback)(const AmrScmPlusMsg * msg)) {
    scmPlusMsgCallback = callback;
}

void registerIdmMsgCallback(void (*callback)(const AmrIdmMsg * msg)) {
    idmMsgCallback = callback;
}

