/*! @file bsp.h
 * @brief This file contains application specific definitions and includes.
 *
 * @b COPYRIGHT
 * @n Silicon Laboratories Confidential
 * @n Copyright 2012 Silicon Laboratories, Inc.
 * @n http://www.silabs.com
 */

#ifndef BSP_H
#define BSP_H

/*------------------------------------------------------------------------*/
/*            Application specific global definitions                     */
/*------------------------------------------------------------------------*/
/*! Platform definition */
/* Note: Plaform is defined in Silabs IDE project file as
 * a command line flag for the compiler. */
//#define SILABS_PLATFORM_WMB930

/*! Extended driver support 
 * Known issues: Some of the example projects 
 * might not build with some extended drivers 
 * due to data memory overflow */

// These only need to be enabled for debugging
// #define RADIO_DRIVER_EXTENDED_SUPPORT
// #define RADIO_DRIVER_FULL_SUPPORT

/*------------------------------------------------------------------------*/
/*            Application specific includes                               */
/*------------------------------------------------------------------------*/


#ifdef ESP8266
#include <user_config.h>
#include <osapi.h>
#include <user_interface.h>
#include <ets_sys.h>

#include "./compiler_defs.h"
#include "./hardware_defs.h"

#include "../../amr.h"
#include "../platform/esp8266/amr_hal.h"
#include "../platform/esp8266/fastgpio.h"
#include "../driver/spi.h"

#else
#include <stdio.h> // printf
#include <unistd.h> // usleep
#endif

#include "../radio/radio.h"


#ifdef CONFIG_DIRECT_TX
#include "./radio_config_si4463_direct_tx_v0-1.h"
#else
#include "./radio_config_si4463_direct_rx_v0-1.h"
#endif

#include "../radio/radio_hal.h"
#include "../radio/radio_comm.h"

#ifdef SILABS_RADIO_SI446X
#include "../radio/Si446x/si446x_api_lib.h"
#include "../radio/Si446x/si446x_defs.h"
#include "../radio/Si446x/si446x_nirq.h"
//#include "drivers/radio/Si446x/si446x_patch.h"
#endif

#ifdef SILABS_RADIO_SI4455
#include "../radio/Si4455/si4455_api_lib.h"
#include "../radio/Si4455/si4455_defs.h"
#include "../radio/Si4455/si4455_nirq.h"
#endif

#endif //BSP_H
