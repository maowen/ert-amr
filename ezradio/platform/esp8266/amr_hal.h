#ifndef AMR_HAL_H
#define AMR_HAL_H

#include <stdint.h>

void amrHalInit();
uint8_t amrHalRunning();
void amrHalEnable(uint8_t enable);

#define usleep ets_delay_us // Override the default usleep definition

#endif
