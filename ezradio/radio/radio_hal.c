/*!
 * File:
 *  radio_hal.c
 *
 * Description:
 *  This file contains RADIO HAL.
 *
 * Silicon Laboratories Confidential
 * Copyright 2011 Silicon Laboratories, Inc.
 */

                /* ======================================= *
                 *              I N C L U D E              *
                 * ======================================= */

#include "../include/bsp.h"
#include <string.h>


                /* ======================================= *
                 *          D E F I N I T I O N S          *
                 * ======================================= */

                /* ======================================= *
                 *     G L O B A L   V A R I A B L E S     *
                 * ======================================= */

                /* ======================================= *
                 *      L O C A L   F U N C T I O N S      *
                 * ======================================= */

                /* ======================================= *
                 *     P U B L I C   F U N C T I O N S     *
                 * ======================================= */

void radio_hal_AssertShutdown(void)
{
#if (defined SILABS_PLATFORM_RFSTICK) || (defined SILABS_PLATFORM_LCDBB) || (defined SILABS_PLATFORM_WMB)
  RF_PWRDN = 1;
#elif (defined ESP8266)
  RF_SDN_ASSERT;
#else
  PWRDN = 1;
#endif
}

void radio_hal_DeassertShutdown(void)
{
#if (defined SILABS_PLATFORM_RFSTICK) || (defined SILABS_PLATFORM_LCDBB) || (defined SILABS_PLATFORM_WMB)
  RF_PWRDN = 0;
#elif (defined ESP8266)
  RF_SDN_DEASSERT;
#else
  PWRDN = 0;
#endif
}

void radio_hal_ClearNsel(void)
{
#if (defined ESP8266)
    RF_NSEL_LOW;
#else
    RF_NSEL = 0;
#endif
}

void radio_hal_SetNsel(void)
{
#if (defined ESP8266)
    RF_NSEL_HIGH;
#else
    RF_NSEL = 1;
#endif
}

BIT radio_hal_NirqLevel(void)
{
    return RF_NIRQ;
}

void radio_hal_SpiWriteByte(U8 byteToWrite)
{
#if (defined SILABS_PLATFORM_RFSTICK) || (defined SILABS_PLATFORM_LCDBB) || (defined SILABS_PLATFORM_WMB)
  bSpi_ReadWriteSpi1(byteToWrite);
#elif (defined ESP8266)
  spi_transfer32(byteToWrite, 8);
#else
  SpiReadWrite(byteToWrite);
#endif
}

U8 radio_hal_SpiReadByte(void)
{
#if (defined SILABS_PLATFORM_RFSTICK) || (defined SILABS_PLATFORM_LCDBB) || (defined SILABS_PLATFORM_WMB)
  return bSpi_ReadWriteSpi1(0xFF);
#elif (defined ESP8266)
  return (U8)spi_transfer32(0xFF, 8);
#else
  return SpiReadWrite(0xFF);
#endif
}

void radio_hal_SpiWriteData(U8 byteCount, U8* pData)
{
#if (defined SILABS_PLATFORM_RFSTICK) || (defined SILABS_PLATFORM_LCDBB) || (defined SILABS_PLATFORM_WMB)
  vSpi_WriteDataSpi1(byteCount, pData);
#elif (defined ESP8266)
  /* spi_transfer(pData, byteCount); */
  U8 cnt;
  for (cnt = 0; cnt < byteCount; ++cnt) {
      spi_transfer32(*(pData++), 8);
  }
#else
  SpiWriteData(byteCount, pData);
#endif
}

void radio_hal_SpiWriteConstData(U8 byteCount, const U8* pData)
{
#if (defined SILABS_PLATFORM_RFSTICK) || (defined SILABS_PLATFORM_LCDBB) || (defined SILABS_PLATFORM_WMB)
  vSpi_WriteDataSpi1(byteCount, pData);
#elif (defined ESP8266)
  U8 cnt;
  for (cnt = 0; cnt < byteCount; ++cnt) {
      spi_transfer32(*(pData++), 8);
  }
#else
  SpiWriteData(byteCount, pData);
#endif
}

void radio_hal_SpiReadData(U8 byteCount, U8* pData)
{
#if (defined SILABS_PLATFORM_RFSTICK) || (defined SILABS_PLATFORM_LCDBB) || (defined SILABS_PLATFORM_WMB)
  vSpi_ReadDataSpi1(byteCount, pData);
#elif (defined ESP8266)
  memset(pData, 0xFF, byteCount);
  spi_transfer(pData, byteCount);
#else
  SpiReadData(byteCount, pData);
#endif
}

#ifdef RADIO_DRIVER_EXTENDED_SUPPORT
BIT radio_hal_Gpio0Level(void)
{
  BIT retVal = FALSE;

#ifdef SILABS_PLATFORM_DKMB
  retVal = FALSE;
#endif
#ifdef SILABS_PLATFORM_UDP
  retVal = EZRP_RX_DOUT;
#endif
#if (defined SILABS_PLATFORM_RFSTICK) || (defined SILABS_PLATFORM_LCDBB)
  retVal = RF_GPIO0;
#endif
#if (defined SILABS_PLATFORM_WMB930)
  retVal = FALSE;
#endif
#if defined (SILABS_PLATFORM_WMB912)
  #ifdef SILABS_IO_WITH_EXTENDER
    //TODO
    retVal = FALSE;
  #endif
#endif

  return retVal;
}

BIT radio_hal_Gpio1Level(void)
{
  BIT retVal = FALSE;

#ifdef SILABS_PLATFORM_DKMB
  retVal = FSK_CLK_OUT;
#endif
#ifdef SILABS_PLATFORM_UDP
  retVal = FALSE; //No Pop
#endif
#if (defined SILABS_PLATFORM_RFSTICK) || (defined SILABS_PLATFORM_LCDBB) || (defined SILABS_PLATFORM_WMB930)
  retVal = RF_GPIO1;
#endif
#if defined (SILABS_PLATFORM_WMB912)
  #ifdef SILABS_IO_WITH_EXTENDER
    //TODO
    retVal = FALSE;
  #endif
#endif

  return retVal;
}

BIT radio_hal_Gpio2Level(void)
{
  BIT retVal = FALSE;

#ifdef SILABS_PLATFORM_DKMB
  retVal = DATA_NFFS;
#endif
#ifdef SILABS_PLATFORM_UDP
  retVal = FALSE; //No Pop
#endif
#if (defined SILABS_PLATFORM_RFSTICK) || (defined SILABS_PLATFORM_LCDBB) || (defined SILABS_PLATFORM_WMB930)
  retVal = RF_GPIO2;
#endif
#if defined (SILABS_PLATFORM_WMB912)
  #ifdef SILABS_IO_WITH_EXTENDER
    //TODO
    retVal = FALSE;
  #endif
#endif

  return retVal;
}

BIT radio_hal_Gpio3Level(void)
{
  BIT retVal = FALSE;

#if (defined SILABS_PLATFORM_RFSTICK) || (defined SILABS_PLATFORM_LCDBB) || (defined SILABS_PLATFORM_WMB930)
  retVal = RF_GPIO3;
#elif defined (SILABS_PLATFORM_WMB912)
  #ifdef SILABS_IO_WITH_EXTENDER
    //TODO
    retVal = FALSE;
  #endif
#endif

  return retVal;
}

#endif
