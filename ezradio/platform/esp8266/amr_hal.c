#include "amr_hal.h"
#include "../../../amr.h"
#include "../../include/bsp.h"

// ESP-12 PINOUT
// RESET        D1/TX0
// ADC          D3/RX0
// CH_PD        D5/SCL
// D16          D4/SDA
// D14/SCLK     D0
// D12/MISO     D2
// D13/MOSI     D15/SS
// VCC          GND

// Si446x Connections
//
// SDN->D2 + 10K Pull-up
// ~NSEL->D15/SS + 10K Pull-down
// SDI->D13/MOSI
// SDO->D12/MISO
// SCLK->D14/SCLK
// ~NIRQ
//
// GND->GND
// VCC->VCC
// GPIO0->D4
// GPIO1->
// GPIO2->D5
// GPIO3->
//

LOCAL void gpio_intr_handler(uint32 intr_mask, void *arg) {

    ETS_GPIO_INTR_DISABLE();
    uint32_t gpio_status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);

    uint8_t cur_bit = RF_RX_DATA;
    amrProcessRxBit(cur_bit);

    /* GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_status); */
    GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, 0xffffffff);
    ETS_GPIO_INTR_ENABLE();
}

void amrHalInit() {
    os_printf("Perform AMR radio init\n");

    gpio_init();
    spi_init();
    spi_mode(0, 0);
    RF_NSEL_INIT;
    RF_NSEL_HIGH;
    RF_SDN_INIT;
    RF_NIRQ_INIT;

    // Configure radio rx data interrupt handler
    ETS_GPIO_INTR_DISABLE();
    ETS_GPIO_INTR_ATTACH(gpio_intr_handler, NULL);

    RF_RX_CLK_INIT;
    RF_RX_DATA_INIT;
    gpio_pin_intr_state_set(RF_RX_CLK_ID_PIN,GPIO_PIN_INTR_POSEDGE);

    si446x_disp_func_info();
    vRadio_Init();
    si446x_disp_func_info();
    si446x_disp_dev_state();
    si446x_disp_gpio_pin_cfg();

    os_printf("Start RX...\n");
    vRadio_StartRX(0);

    ETS_GPIO_INTR_ENABLE(); /* Enable GPIO interrupts */
}

