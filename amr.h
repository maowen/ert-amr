#ifndef AMR_LIB_H
#define AMR_LIB_H

#include <stdint.h>
#include "ring/ringbuf.h"
#include <stdio.h>

#define AMR_MSG_SCM_RAW_SIZE 12
#define AMR_MSG_SCM_PLUS_RAW_SIZE 16
#define AMR_MSG_IDM_RAW_SIZE 92
#define AMR_MAX_MSG_SIZE AMR_MSG_IDM_RAW_SIZE
#define AMR_MSG_HDR_SIZE sizeof(AmrMsgHeader)

#define debug_printf(fmt, ...) \
    do { if (AMR_DEBUG) printf("%s:%d:%s(): " fmt, __FILE__, \
            __LINE__, __func__, ##__VA_ARGS__); } while (0)

typedef enum  {
    AMR_MSG_TYPE_SCM = 0,
    AMR_MSG_TYPE_SCM_PLUS,
    AMR_MSG_TYPE_IDM,
    AMR_MSG_TYPE_IDM18
} AMR_MSG_TYPE;

#pragma pack(push, 1)
typedef struct {
    uint32_t id;
    uint8_t type;
    uint8_t tamper_phy;
    uint8_t tamper_enc;
    uint32_t consumption;
    uint16_t crc;
} AmrScmMsg;

typedef struct {
    uint16_t frameSync;
    uint8_t protocolId;
    uint8_t endpointType;
    uint32_t endpointId;
    uint32_t consumption;
    uint16_t tamper;
    uint16_t crc;
} AmrScmPlusMsg;

typedef struct {
    uint8_t moduleProgrammingState;
    uint8_t tamperCounters[6];
    uint16_t asyncCnt;
    uint8_t powerOutageFlags[6];
    uint32_t lastConsumption;
    uint16_t differentialConsumption[53];
} IdmStdData;

typedef struct {
    uint8_t unknown[10];
    uint32_t lastConsumption;
    uint32_t lastExcess;
    uint32_t lastResidual;
    uint32_t lastConsumptionHighRes;
    uint16_t differentialConsumption[27];
} IdmX18Data;

typedef struct {
    uint32_t preamble;
    uint8_t packetTypeID;
    uint8_t packetLength;
    uint8_t hammingCode;
    uint8_t appVersion;
    uint8_t ertType;
    uint32_t ertId;
    uint8_t consumptionIntervalCount;
    union {
        IdmStdData std;
        IdmX18Data x18;
    } data;
    uint16_t txTimeOffset;
    uint16_t serialNumberCRC;
    uint16_t pktCRC;
} AmrIdmMsg;

typedef struct {
    AMR_MSG_TYPE type;
    uint32_t timestamp;
    uint8_t bitOffset;
} AmrMsgHeader;
#pragma pack(pop)

void amrInit();
void amrEnable(uint8_t enable);
uint8_t amrRunning();
static void amrProcessRxBit(uint8_t rxBit);
void amrProcessMsgs();
void printAmrMsg(const char* dateStr, const void * msg, AMR_MSG_TYPE msgType);
void registerAmrMsgCallback(void (*callback)(const void * msg, AMR_MSG_TYPE msgType, const uint8_t * data));

void printScmMsg(const char* dateStr, const AmrScmMsg * msg);
void printScmPlusMsg(const char * dateStr, const AmrScmPlusMsg * msg);
void printIdmMsg(const char * dateStr, const AmrIdmMsg * msg);
void registerScmMsgCallback(void (*callback)(const AmrScmMsg * msg));
void registerScmPlusMsgCallback(void (*callback)(const AmrScmPlusMsg * msg));
void registerIdmMsgCallback(void (*callback)(const AmrIdmMsg * msg));

#endif