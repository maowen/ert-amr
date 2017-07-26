#ifndef AMR_HAL_H
#define AMR_HAL_H

void amrHalInit();

#define printf os_printf // Override the default printf definition
#define debug_printf(fmt, ...) \
    do { if (DEBUG) printf("%s:%d:%s(): " fmt, __FILE__, \
            __LINE__, __func__, ##__VA_ARGS__); } while (0)
#define usleep os_delay_us // Override the default usleep definition

#endif
