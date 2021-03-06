/*! @file radio.c
 * @brief This file contains functions to interface with the radio chip.
 *
 * @b COPYRIGHT
 * @n Silicon Laboratories Confidential
 * @n Copyright 2012 Silicon Laboratories, Inc.
 * @n http://www.silabs.com
 */

#include "../include/bsp.h"

/*****************************************************************************
 *  Local Macros & Definitions
 *****************************************************************************/

/*****************************************************************************
 *  Global Variables
 *****************************************************************************/
const SEGMENT_VARIABLE(Radio_Configuration_Data_Array[], U8, SEG_CODE) = \
              RADIO_CONFIGURATION_DATA_ARRAY;

const SEGMENT_VARIABLE(RadioConfiguration, tRadioConfiguration, SEG_CODE) = \
                        RADIO_CONFIGURATION_DATA;

const SEGMENT_VARIABLE_SEGMENT_POINTER(pRadioConfiguration, tRadioConfiguration, SEG_CODE, SEG_CODE) = \
                        &RadioConfiguration;

/*****************************************************************************
 *  Local Function Declarations
 *****************************************************************************/
void ICACHE_FLASH_ATTR vRadio_PowerUp(void);
/*!
 *  Power up the Radio.
 *
 *  @note
 *
 */
void ICACHE_FLASH_ATTR vRadio_PowerUp(void)
{
  /* SEGMENT_VARIABLE(wDelay,  U16, SEG_XDATA) = 0u; */
  /* SEGMENT_VARIABLE(lBootOpt, U8, SEG_XDATA) = 0u; */

  /* Hardware reset the chip */
  si446x_reset();

  usleep(10000);
  /* Wait until reset timeout or Reset IT signal */
  /* for (; wDelay < pRadioConfiguration->Radio_Delay_Cnt_After_Reset; wDelay++); */
}

/*!
 *  Radio Initialization.
 *
 *  @author Sz. Papp
 *
 *  @note
 *
 */
void ICACHE_FLASH_ATTR vRadio_Init(void)
{
  /* Power Up the radio chip */
  debug_printf("Radio POR\n");
  vRadio_PowerUp();

  debug_printf("Powering up radio... ");
  si446x_power_up(0x01, 0x00, RADIO_CONFIGURATION_DATA_RADIO_XO_FREQ);
  debug_printf("Done\n");
  /* si446x_disp_dev_state();  */
  debug_printf("Get INT status... ");
  si446x_get_int_status(0x00, 0x00, 0x00);
  debug_printf("Done\n");
  /* si446x_disp_dev_state(); */

  /* Load radio configuration */
  while (SI446X_SUCCESS != si446x_configuration_init(pRadioConfiguration->Radio_ConfigurationArray))
  {
    /* Error hook */
/*
#if !(defined SILABS_PLATFORM_WMB912)
    LED4 = !LED4;
#else
    vCio_ToggleLed(0x04);
#endif
*/
    usleep(10000);
    /* Power Up the radio chip */
    vRadio_PowerUp();
  }
  debug_printf("Completed EZ Config!\n");


  // Read ITs, clear pending ones
  si446x_get_int_status(0u, 0u, 0u);
}

/*!
 *  Set Radio to RX mode, fixed packet length.
 *
 *  @param channel Freq. Channel
 *
 *  @note
 *
 */
void ICACHE_FLASH_ATTR vRadio_StartRX(U8 channel)
{
  // Read ITs, clear pending ones
  si446x_get_int_status(0u, 0u, 0u);

  /* Start Receiving packet, channel 0, START immediately, Packet off  */
  si446x_start_rx(channel, 0u, 0u,
                  SI446X_CMD_START_RX_ARG_NEXT_STATE1_RXTIMEOUT_STATE_ENUM_NOCHANGE,
                  SI446X_CMD_START_RX_ARG_NEXT_STATE2_RXVALID_STATE_ENUM_RX,
                  SI446X_CMD_START_RX_ARG_NEXT_STATE3_RXINVALID_STATE_ENUM_RX );
}

/*!
 *  Set Radio to TX mode, fixed packet length.
 *
 *  @param channel Freq. Channel, Packet to be sent
 *
 *  @note
 *
 */
void ICACHE_FLASH_ATTR vRadio_StartTx(U8 channel, U8 *pioFixRadioPacket)
{
  // Read ITs, clear pending ones
  si446x_get_int_status(0u, 0u, 0u);

  /* Start sending packet on channel, START immediately, Packet according to PH */
  si446x_start_tx(channel, 0u, 0u);
}
