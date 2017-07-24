#include <stdint.h>
#include "ring/ringbuf.h"

#define AMR_MSG_SCM_RAW_SIZE 12
#define AMR_MSG_IDM_RAW_SIZE 92

typedef enum  {
    AMR_MSG_TYPE_SCM = 0,
    AMR_MSG_TYPE_IDM = 1
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
    uint32_t preamble;
    uint8_t packetTypeID;
    uint8_t packetLength;
    uint8_t hammingCode;
    uint8_t appVersion;
    uint8_t ertType;
    uint32_t ertId;
    uint8_t consumptionIntervalCount;
    uint8_t moduleProgrammingState;
    uint8_t tamperCounters[6];
    uint16_t asyncCnt;
    uint8_t powerOutageFlags[6];
    uint32_t lastConsumption;
    uint16_t differentialConsumption[53];
    uint16_t txTimeOffset;
    uint16_t serialNumberCRC;
    uint16_t pktCRC;
} AmrIdmMsg;

typedef struct {
    AMR_MSG_TYPE type;
    uint32_t timestamp;
} AmrMsgHeader;
#pragma pack(pop)

void amrInit();
void amrProcessRxBit(uint8_t rxBit);
void amrProcessMsgs();
void printIdmMsg(const AmrIdmMsg * msg);
void printScmMsg(const AmrScmMsg * msg);
void registerScmMsgCallback(void (*callback)(const AmrScmMsg * msg));
void registerIdmMsgCallback(void (*callback)(const AmrIdmMsg * msg));
