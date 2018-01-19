#ifndef HARDWARE_DEFS_H
#define HARDWARE_DEFS_H

/*-------------------------------------------------------------*/
/*						      Global definitions				                 */
/*-------------------------------------------------------------*/

#if (defined ESP8266)
#include "../platform/esp8266/fastgpio.h"

#define RF_SDN_INIT GPIO2_OUTPUT_SET
#define RF_SDN_ASSERT GPIO2_H
#define RF_SDN_DEASSERT GPIO2_L

/* START DIRECT TX MODE DEFINES */
#define RF_TX_CLK_INIT GPIO4_INPUT_SET
#define RF_TX_CLK GPIO4_IN
#define RF_TX_CLK_ID_PIN GPIO_ID_PIN(4)

#define RF_TX_DATA_INIT GPIO5_OUTPUT_SET
#define RF_TX_DATA_HIGH GPIO5_H
#define RF_TX_DATA_LOW GPIO5_L
/* END DIRECT TX MODE */

/* START DIRECT RX MODE DEFINES */
#define RF_RX_CLK_INIT GPIO4_INPUT_SET
#define RF_RX_CLK GPIO4_IN
#define RF_RX_CLK_ID_PIN GPIO_ID_PIN(4)

#define RF_RX_DATA_INIT GPIO5_INPUT_SET
#define RF_RX_DATA GPIO5_IN
/* END DIRECT RX MODE */

#define RF_NSEL_INIT GPIO15_OUTPUT_SET
#define RF_NSEL_LOW GPIO15_L
#define RF_NSEL_HIGH GPIO15_H

#define RF_NIRQ_INIT GPIO16_INPUT_SET
#define RF_NIRQ GPIO16_IN

#else
#error Other platforms are not supported yet!
#endif

#endif //HARDWARE_DEFS_H
