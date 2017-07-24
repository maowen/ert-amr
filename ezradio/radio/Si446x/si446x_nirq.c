/*
 * Silicon Laboratories Confidential
 * Copyright 2011 Silicon Laboratories, Inc.
 *
 * THIS FILE IS AUTOMATICALLY GENERATED. DO NOT EDIT!
 */

#include "../../include/bsp.h"


//SEGMENT_VARIABLE( Si446xChipPend, U8, SEG_BDATA );
//SBIT(Si446xWUTPend,Si446xChipPend,0);
//SBIT(Si446xLowBattPend,Si446xChipPend,1);
//SBIT(Si446xChipReadyPend,Si446xChipPend,2);
//SBIT(Si446xCmdErrPend,Si446xChipPend,2);
//SBIT(Si446xStateChangePend,Si446xChipPend,2);
//SBIT(Si446xFifoUnderflowOverflowErrorPend,Si446xChipPend,2);
//SEGMENT_VARIABLE( Si446xPhPend, U8, SEG_BDATA );
//SEGMENT_VARIABLE( Si446xModemPend, U8, SEG_BDATA );

#define Si446xWUTPend                           Si446xPhPend0
#define Si446xLowBattPend                       Si446xPhPend1
#define Si446xChipReadyPend                     Si446xPhPend2
#define Si446xCmdErrPend                        Si446xPhPend3
#define Si446xStateChangePend                   Si446xPhPend4
#define Si446xFifoUnderflowOverflowErrorPend    Si446xPhPend5
#define Si446xDummyPhPend6                      Si446xPhPend6
#define Si446xDummyPhPend7                      Si446xPhPend7

#define Si446xSyncDetectPend                    Si446xModemPend0
#define Si446xPreambleDetectPend                Si446xModemPend1
#define Si446xInvalidPreamblePend               Si446xModemPend2
#define Si446xRssiPend                          Si446xModemPend3
#define Si446xRssiJumpPend                      Si446xModemPend4
#define Si446xInvalidSyncPend                   Si446xModemPend5
#define Si446xDummyModemPend6                   Si446xModemPend6
#define Si446xDummyModemPend7                   Si446xModemPend7

#define Si446xRxFifoAlmostFullPend              Si446xChipPend0
#define Si446xTxFifoAlmostEmptyPend             Si446xChipPend1
#define Si446xCrc16ErrorPend                    Si446xChipPend2
#define Si446xCrc32ErrorPend                    Si446xChipPend3
#define Si446xPacketRxPend                      Si446xChipPend4
#define Si446xPacketSentPend                    Si446xChipPend5
#define Si446xFilterMissPend                    Si446xChipPend6
#define Si446xFilterMatchPend                   Si446xChipPend7

/*!
 * Do NOT use SEG_BDATA as that is not understood by SDCC!
 * Use BITS, WRITE_TO_BIT_ARRAY and READ_FROM_BIT_ARRAY instead to allocate and
 * handle bit arrays in a convenient way.
 *
 * Allocate all 8bits if WRITE_TO_BIT_ARRAY and READ_FROM_BIT_ARRAY macros
 * are to be used. Otherwise these macros would read/write bits that are
 * placed within that byte by the linker.
 */
BITS(Si446xPhPend, 0);
BITS(Si446xPhPend, 1);
BITS(Si446xPhPend, 2);
BITS(Si446xPhPend, 3);
BITS(Si446xPhPend, 4);
BITS(Si446xPhPend, 5);
BITS(Si446xPhPend, 6);
BITS(Si446xPhPend, 7);

BITS(Si446xModemPend, 0);
BITS(Si446xModemPend, 1);
BITS(Si446xModemPend, 2);
BITS(Si446xModemPend, 3);
BITS(Si446xModemPend, 4);
BITS(Si446xModemPend, 5);
BITS(Si446xModemPend, 6);
BITS(Si446xModemPend, 7);

BITS(Si446xChipPend, 0);
BITS(Si446xChipPend, 1);
BITS(Si446xChipPend, 2);
BITS(Si446xChipPend, 3);
BITS(Si446xChipPend, 4);
BITS(Si446xChipPend, 5);
BITS(Si446xChipPend, 6);
BITS(Si446xChipPend, 7);

/*!
 * This function is used to handle the assertion of nIRQ of the si446x chip.
 *
 * @note This function can take up to 6 ms depending on the startup time of the
 * si446x chip.
 *
 */
void si446x_nirq_process(void)
{
#ifdef SI446X_USER_CONFIG_USE_FRR_ABC_FOR_NIRQ
    SEGMENT_VARIABLE(pendBits[3], U8, SEG_DATA);

    si446x_frr_read();
#else
    si446x_get_int_status(0, 0, 0);
//    Si446xPhPend = Si446xCmd.GET_INT_STATUS.PH_PEND;
//    Si446xModemPend = Si446xCmd.GET_INT_STATUS.MODEM_PEND;
//    Si446xChipPend = Si446xCmd.GET_INT_STATUS.CHIP_PEND;
    WRITE_TO_BIT_ARRAY(Si446xPhPend, Si446xCmd.GET_INT_STATUS.PH_PEND);
    WRITE_TO_BIT_ARRAY(Si446xModemPend, Si446xCmd.GET_INT_STATUS.MODEM_PEND);
    WRITE_TO_BIT_ARRAY(Si446xChipPend, Si446xCmd.GET_INT_STATUS.CHIP_PEND);
#endif
}
