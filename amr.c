#include "amr.h"
#include <string.h>

#ifndef AMR_DEBUG
#define AMR_DEBUG 0
#endif

#ifdef ESP8266
// Include AMR HAL source file so compiler can inline the
// amrProcessRxBit function into the interrupt handler
#include "ezradio/platform/esp8266/amr_hal.c"
#include <osapi.h>
#else
#include <stdio.h>
void amrHalInit() {}
void amrHalEnable(uint8_t enable) {}
uint8_t amrHalRunning() { return 1; }
#define ICACHE_FLASH_ATTR
#endif


#define RX_BUF_SIZE (AMR_MSG_HDR_SIZE + 2*AMR_MAX_MSG_SIZE)
#define PROC_RING_BUF_SIZE 512

#define SCM_PRE_32      0xf9530000
#define SCM_PRE_32_MASK 0xfffff800
#define SCM_PLUS_PRE_32      0x16a30000
#define SCM_PLUS_PRE_32_MASK 0xffff0000
#define IDM_PRE_32      0x555516a3
#define IDM_PRE_32_MASK 0xffffffff

uint32_t minIntTime = 999999;
uint32_t maxIntTime = 0;

static uint8_t rxBuf0[RX_BUF_SIZE] = {0}; //! Manchester decoded data buffer 0
static uint8_t rxBuf1[RX_BUF_SIZE] = {0}; //! Manchester decoded data buffer 1
static uint8_t *rxBuf = rxBuf0; //! Active data bufer
static uint16_t rxBufHeadBit = 0; //! Head of circular buffer in bits
static uintptr_t xor_rxBufPtr = 0; //! XOR-ed pointers of buffers 0 and 1 for fast toggling

static uint8_t prevRxBit = 0;
static AmrScmMsg scmMsg = {0};
static AmrScmPlusMsg scmPlusMsg = {0};
static AmrIdmMsg idmMsg = {0};

static void (*amrMsgCallback)(const void * msg, AMR_MSG_TYPE msgType, const uint8_t *data) = NULL;

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
    // system_set_os_print(1);
    msgRing = ringInit(msgRingData, sizeof(msgRingData));
	xor_rxBufPtr = (uintptr_t)(rxBuf0) ^ (uintptr_t)(rxBuf1);
    amrHalInit();
}

void amrEnable(uint8_t enable) {
    amrHalEnable(enable);
}

uint8_t amrRunning() {
    return amrHalRunning();
}

// This function needs to be inlined into the interrupt handler for performance reasons
static inline void amrProcessRxBit(uint8_t rxBit) {
    // The decoding of the current bit depends on the previous bit and the
    // result is stored in alternating buffers because there are two possible
    // alignments of the encoded data.
    
    /* uint32_t ts = system_get_time(); */

    // Select active buffer of manchester decoded data
    /* rxBuf = (rxBuf != rxBuf0) ? rxBuf0 : rxBuf1; */
    // Toggle between buffer pointers with XOR-ed value
    rxBuf = (uint8_t*)(xor_rxBufPtr ^ (uintptr_t)rxBuf);

    // Manchester decode the current bit based on the previous bit
    // Decode 0b10 as 1 and 0b01, 0b00, 0b11 as 0
    uint8_t manchBit = prevRxBit && !rxBit;

    // Leave space at the front of the Rx buffer to populate the message header
    uint8_t* bufHead = rxBuf + AMR_MSG_HDR_SIZE + (rxBufHeadBit / 8);
    uint8_t bitOffset = (rxBufHeadBit % 8);
    uint8_t nthBit = 7 - bitOffset;

    // All rx buffer write are duplicated to an identical circular buffer at the
    // back of the first buffer to avoid having to unwrap data;
    *bufHead = (*bufHead & (~(1u << nthBit))) | (manchBit << nthBit);
    *(bufHead + AMR_MAX_MSG_SIZE) = *bufHead;

    uint8_t* msgEnd = bufHead + AMR_MAX_MSG_SIZE;
    uint8_t* scmData = msgEnd - AMR_MSG_SCM_RAW_SIZE;
    uint8_t* idmData = msgEnd - AMR_MSG_IDM_RAW_SIZE;

    // Delay the seach for SCM+ preamble until after we checked for an IDM
    // message. This is accomplished by using the same pointer for both of them.
    // This will result in an SCM+ preamble being found 16-bits after every IDM
    // message. Populating the SCM+ header in front of the SCM+ preamble when the
    // current messsage is an IDM message the IDM message.
    int8_t* scmPlusData = idmData;

    // Assemble the 32bits of data at the message starts for comparision
    // Compute preambles
    uint32_t scmPre =
        (scmData[0] << (24+bitOffset)) |
        (scmData[1] << (16+bitOffset)) |
        (scmData[2] << (8+bitOffset)) |
        (scmData[3] << (0+bitOffset)) |
        (scmData[4] >> (8-bitOffset));
    uint32_t idmPre =
        idmData[0] << (24+bitOffset) |
        idmData[1] << (16+bitOffset) |
        idmData[2] << (8+bitOffset) |
        idmData[3] << (0+bitOffset) |
        idmData[4] >> (8-bitOffset);
    uint32_t scmPlusPre = idmPre;


    if ((scmPre & SCM_PRE_32_MASK) == SCM_PRE_32) {
    /* if ((preamble & SCM_PRE_32_MASK) == SCM_PRE_32) { */
        AmrMsgHeader * hdr = (AmrMsgHeader *)(scmData - AMR_MSG_HDR_SIZE);
        hdr->type = AMR_MSG_TYPE_SCM;
        // TODO populate timestamp with platform agnostic call
        // hdr->timestamp = system_get_time() / 1000; // Convert micro to millis
        hdr->bitOffset = bitOffset;
        RING_STATUS status =
            ringPush(&msgRing, (uint8_t*)hdr,
                    AMR_MSG_SCM_RAW_SIZE + AMR_MSG_HDR_SIZE + 1);
        if (AMR_DEBUG && status != RING_STATUS_OK) {
            debug_printf("Failed to push SCM msg onto ring. Status: %u\r\n", status);
        }
    }
    else if (idmPre == IDM_PRE_32) {
        AmrMsgHeader * hdr = (AmrMsgHeader *)(idmData - AMR_MSG_HDR_SIZE);
        hdr->type = AMR_MSG_TYPE_IDM;
        // TODO populate timestamp with platform agnostic call
        // hdr->timestamp = system_get_time() / 1000; // Convert micro to millis
        hdr->bitOffset = bitOffset;
        RING_STATUS status =
            ringPush(&msgRing, (uint8_t*)hdr,
                    AMR_MSG_IDM_RAW_SIZE + AMR_MSG_HDR_SIZE + 1);
        if (AMR_DEBUG && status != RING_STATUS_OK) {
            debug_printf("Failed to push IDM msg onto ring. Status: %u\r\n", status);
        }
    }
    else if ((scmPlusPre & SCM_PLUS_PRE_32_MASK) == SCM_PLUS_PRE_32) {
        AmrMsgHeader * hdr = (AmrMsgHeader *)(scmPlusData - AMR_MSG_HDR_SIZE);
        hdr->type = AMR_MSG_TYPE_SCM_PLUS;
        // TODO populate timestamp with platform agnostic call
        // hdr->timestamp = system_get_time() / 1000; // Convert micro to millis
        hdr->bitOffset = bitOffset;
        RING_STATUS status =
            ringPush(&msgRing, (uint8_t*)hdr,
                    AMR_MSG_SCM_PLUS_RAW_SIZE + AMR_MSG_HDR_SIZE + 1);
        if (AMR_DEBUG && status != RING_STATUS_OK) {
            debug_printf("Failed to push SCM+ msg onto ring. Status: %u\r\n", status);
        }
    }

    prevRxBit = rxBit;

    // Only increment the head bit after we've processed both buffers 0 and 1
    if (rxBuf == rxBuf1) {
        rxBufHeadBit = (rxBufHeadBit + 1) % (AMR_MAX_MSG_SIZE*8);
    }

    /* uint32_t dt = system_get_time() - ts; */
    /* if (dt < minIntTime) minIntTime = dt; */
    /* if (dt > maxIntTime) maxIntTime = dt; */
}

uint32_t extractBits(const uint8_t *data, uint16_t offset, uint16_t len) {
    uint32_t out = 0;
    uint16_t i = offset / 8;    // Starting byte
    uint16_t startBit = offset % 8;  // Starting bit within starting byte
    uint16_t unusedBits = (8 - startBit);
    uint8_t mask = (0xff >> startBit);   // Starting byte bitmask

    if (len <= unusedBits) {
        out = (data[i] & mask) >> (unusedBits - len);
    }
    else {
        out = data[i] & mask;
        len -= unusedBits;

        while (len > 0) {
            ++i;
            if (len <= 8) {
                out = (out << len) | (data[i] >> (8 - len));
                break;
            }
            else {
                out = (out << 8) | data[i];
                len -= 8;
            }
        }
    }

    return out;
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


/*
if((scmMsg.id & 0xfffffff0) ==  (32839945 & 0xfffffff0)) {
        printf("SCM %8u: 0x%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\r\n",
        scmMsg.id,
        data[0], data[1], data[2], data[3], data[4], data[5], data[6],
        data[7], data[8], data[9], data[10], data[11], data[12]);
}
*/

        if (amrMsgCallback) {
            amrMsgCallback(&scmMsg, AMR_MSG_TYPE_SCM, data);
        }
    }
    else {
        debug_printf("INAVLID SCM CHECKSUM\r\n");
    }
}

static inline void parseSCMPlusMsg(const uint8_t *data, uint32_t t_ms) {
    if(computeCCITTCRC(data+2, sizeof(AmrScmPlusMsg)-2)) {
        memcpy((void*)&scmPlusMsg, (void*)data, sizeof(AmrScmPlusMsg));
        scmPlusMsg.frameSync = NTOH_16BIT(scmPlusMsg.frameSync);
        scmPlusMsg.protocolId = scmPlusMsg.protocolId; // No op
        scmPlusMsg.endpointType = scmPlusMsg.endpointType; // No op
        scmPlusMsg.endpointId = NTOH_32BIT(scmPlusMsg.endpointId);
        scmPlusMsg.consumption = NTOH_32BIT(scmPlusMsg.consumption);
        scmPlusMsg.tamper = NTOH_16BIT(scmPlusMsg.tamper);
        scmPlusMsg.crc = NTOH_16BIT(scmPlusMsg.crc);


        if (amrMsgCallback) {
            amrMsgCallback(&scmPlusMsg, AMR_MSG_TYPE_SCM_PLUS, data);
        }
    }
    else {
        debug_printf("INAVLID SCM+ CHECKSUM\r\n");
    }
}

static inline void parseIDMMsg(const uint8_t *data, uint32_t t_ms) {
    const uint8_t * head = data;
    if (computeCCITTCRC(head+4, 88)) {
        memcpy((void*)&idmMsg, (void*)head, 14);
        head+=14;

        idmMsg.preamble = NTOH_32BIT(idmMsg.preamble);
        // idmMsg.ertType; // No op (deviation from rtl-amr that masks out the first 4-bits)
        idmMsg.ertId = NTOH_32BIT(idmMsg.ertId);

        if (idmMsg.ertType == 0x18) {
            memcpy((void *)&(idmMsg.data.x18.unknown), (void*)head, 10);
            head += 10;
            memcpy((void *)&(idmMsg.data.x18.lastConsumption), (void*)head, 4);
            idmMsg.data.x18.lastConsumption = NTOH_32BIT(idmMsg.data.x18.lastConsumption);
            head += 4;
            idmMsg.data.x18.lastExcess = (head[0] << 16) | (head[1] << 8) | head[0];
            head += 3;
            idmMsg.data.x18.lastResidual = (head[0] << 16) | (head[1] << 8) | head[0];
            head += 3;
            memcpy((void *)&(idmMsg.data.x18.lastConsumptionHighRes), (void*)head, 4);
            idmMsg.data.x18.lastConsumptionHighRes = NTOH_32BIT(idmMsg.data.x18.lastConsumptionHighRes);
            head += 4;

            const uint16_t spacing = 14;
            for (uint16_t cnt = 0; cnt < 27; cnt++) {
                idmMsg.data.x18.differentialConsumption[cnt] = extractBits(head, cnt * spacing, spacing);
            }
            head += 48;
        }
        else {
            memcpy((void *)&(idmMsg.data.std.moduleProgrammingState), (void *)head, 19);
            // idmMsg.tamperCounters; // No op
            head += 19;

            idmMsg.data.std.asyncCnt = NTOH_16BIT(idmMsg.data.std.asyncCnt);
            // idm.powerOutageFlags; // No op
            idmMsg.data.std.lastConsumption = NTOH_32BIT(idmMsg.data.std.lastConsumption);

            const uint16_t spacing = 9;
            for (uint16_t cnt = 0; cnt < 47; cnt++)
            {
                idmMsg.data.std.differentialConsumption[cnt] = extractBits(head, cnt * spacing, spacing);
            }
            head += 53;

        }

        memcpy((void *)&(idmMsg.txTimeOffset), (void *)head, 6);
        head += 6;

        idmMsg.txTimeOffset = NTOH_16BIT(idmMsg.txTimeOffset);
        idmMsg.serialNumberCRC = NTOH_16BIT(idmMsg.serialNumberCRC);
        idmMsg.pktCRC = NTOH_16BIT(idmMsg.pktCRC);


/* Print Binary */
/*
if((idmMsg.ertId & 0xfffffff0) ==  (32839945 & 0xfffffff0)) {
        // printf("IDM %8u: 0x", idmMsg.ertId); 
        printf("\r\n");
        for (int j = 0; j < 92; ++j) {
            printf("%02X", data[j]);
            os_delay_us(2000);
     }
    printf("\r\n");
    os_delay_us(50000);
}
*/

        if (amrMsgCallback) {
            amrMsgCallback(&idmMsg, AMR_MSG_TYPE_IDM, data);
        }
    }
    else {
        debug_printf("INVALID IDM CHECKSUM\r\n");
    }
}

void amrProcessMsgs() {
    while (1) {
        uint8_t * peek = 0;
        RingPos_t size = ringPeek(&msgRing, &peek); 
        // Message available
        if (size > AMR_MSG_HDR_SIZE && peek) {
            AmrMsgHeader* hdr = (AmrMsgHeader*)peek;
            uint8_t* msgData = peek + AMR_MSG_HDR_SIZE;

            // Shift data to be byte boundary aligned
            if (hdr->bitOffset != 0) {
                uint8_t offset = hdr->bitOffset;
                uint8_t i = 0;
                for (; i < (size - AMR_MSG_HDR_SIZE - 1); ++i) {
                    msgData[i] = (msgData[i] << offset) | (msgData[i+1] >> (8-offset));
                }
            }

            switch (hdr->type) {
                case AMR_MSG_TYPE_SCM:
                    {
                        /*printf("SCM Msg. Offset: %u Pre: 0x%02x%02X%02X\r\n",
                                hdr->bitOffset, msgData[0], msgData[1],
                                msgData[2] & 0xf8);*/
                        parseSCMMsg(msgData, hdr->timestamp);
                    }
                    break;
                case AMR_MSG_TYPE_SCM_PLUS:
                    {
                        /*printf("SCM+ Msg. Offset: %u Pre: 0x%02x%02x\r\n",
                                hdr->bitOffset, msgData[0], msgData[1]);*/
                        parseSCMPlusMsg(msgData, hdr->timestamp);
                    }
                    break;
                case AMR_MSG_TYPE_IDM:
                    {
                        /*printf("IDM Msg. Offset: %u Pre: 0x%02x%02x%02x%02x\r\n",
                                hdr->bitOffset, msgData[0], msgData[1],
                                msgData[2], msgData[3]);*/
                        parseIDMMsg(msgData, hdr->timestamp);
                    }
                    break;
                default:
                    {
                        debug_printf("Unhandled message type: %u\r\n", hdr->type);
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

    /*
    printf("%2u %2u\r\n", minIntTime, maxIntTime);
    minIntTime = 999999;
    maxIntTime = 0;
    // */
}

void ICACHE_FLASH_ATTR printIdmMsg(const char * dateStr, const AmrIdmMsg * msg) {
    if (!msg) {
        printf("Error: Invalid IDM message pointer\r\n");
        return;
    }
    
    printf(
        "{Time:%s IDM:{" //Preamble:0x%08X PacketTypeId:0x%02X PacketLength:0x%02X "
        // "HammingCode:0x%02X ApplicationVersion:0x%02X "
        "ERTType:0x%02X ErtID:%10u MsgCnt:%u ",
        dateStr ? dateStr : "N/A",
        // msg->preamble, msg->packetTypeID, msg->packetLength,
        // msg->hammingCode, msg->appVersion,
        msg->ertType, msg->ertId, msg->consumptionIntervalCount);

    if (msg->ertType == 0x18) {
        printf("LastConsumption:%u LastExcess:%u LastResidual:%u LastConsumptionHiRes:%u ",
            msg->data.x18.lastConsumption, msg->data.x18.lastExcess,
            msg->data.x18.lastResidual, msg->data.x18.lastConsumptionHighRes);
        printf("DiffConsump:[");
        uint8_t i = 0;
        while (i < 27) {
            printf(" %u", msg->data.x18.differentialConsumption[i++]);
        }
        printf("] ");
    }
    else {
        printf(
            "ModuleProgrammingState:0x%02X TamperCounters:%02X%02X%02X%02X%02X%02X "
            "AsynchronousCounters:0x%02X PowerOutageFlags:%02X%02X%02X%02X%02X%02X "
            "LastConsumption:%u DifferentialConsumptionIntervals:[",
            msg->data.std.moduleProgrammingState,
            msg->data.std.tamperCounters[0], msg->data.std.tamperCounters[1],
            msg->data.std.tamperCounters[2], msg->data.std.tamperCounters[3],
            msg->data.std.tamperCounters[4], msg->data.std.tamperCounters[5],
            msg->data.std.asyncCnt,
            msg->data.std.powerOutageFlags[0], msg->data.std.powerOutageFlags[1],
            msg->data.std.powerOutageFlags[2], msg->data.std.powerOutageFlags[3],
            msg->data.std.powerOutageFlags[4], msg->data.std.powerOutageFlags[5],
            msg->data.std.lastConsumption);
        printf("%u", msg->data.std.differentialConsumption[0]);
        uint8_t i = 1;
        while (i < 47) {
            printf(" %u", msg->data.std.differentialConsumption[i++]);
        }
        printf("]");
    }

    printf(" TransmitTimeOffset:%u SerialNumberCRC:0x%04X PacketCRC:0x%02X}}\r\n",
            msg->txTimeOffset, msg->serialNumberCRC, msg->pktCRC);
}

void printScmMsg(const char * dateStr, const AmrScmMsg * msg) {
    printf(
        "{Time:%s SCM:{ID:%u Type: %u Tamper:{Phy:%02u Enc:%02u} "
        "Consumption: %7u CRC:0x%04x}}\r\n",
        dateStr ? dateStr : "N/A",
        msg->id, msg->type, msg->tamper_phy,
        msg->tamper_enc, msg->consumption,
        msg->crc);
}

void printScmPlusMsg(const char * dateStr, const AmrScmPlusMsg * msg) {
    printf(
        "{Time:%s SCM+:{ProtocolId:0x%02X "
        "EndpointType:0x%02X EndpointID:%10u Consumption:%10u "
        " Tamper:0x%04X PacketCRC:0x%04x}}\r\n",
        dateStr ? dateStr : "N/A",
        msg->protocolId, msg->endpointType, msg->endpointId,
        msg->consumption, msg->tamper, msg->crc);
}

void printAmrMsg(const char * dateStr, const void * msg, AMR_MSG_TYPE msgType) {
    switch (msgType) {
        case AMR_MSG_TYPE_SCM:
            printScmMsg(dateStr, (AmrScmMsg*)msg);
        break;
        case AMR_MSG_TYPE_SCM_PLUS:
            printScmPlusMsg(dateStr, (AmrScmPlusMsg*)msg);
        break;
        case AMR_MSG_TYPE_IDM:
            printIdmMsg(dateStr, (AmrIdmMsg*)msg);
        break;
        default:
        break;
    }
}

void registerAmrMsgCallback(void (*callback)(const void * msg, AMR_MSG_TYPE msgType, const uint8_t * data)) {
    amrMsgCallback = callback;
}
