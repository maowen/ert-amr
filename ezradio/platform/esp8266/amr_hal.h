#ifndef AMR_HAL_H
#define AMR_HAL_H

#include <stdint.h>

void amrHalInit();
uint8_t amrHalRunning();
void amrHalEnable(uint8_t enable);

#define printf os_printf_plus // Override the default printf definition
#define usleep ets_delay_us // Override the default usleep definition

#endif
