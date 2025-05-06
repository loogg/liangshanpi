#ifndef __LV_PORT_H
#define __LV_PORT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

extern uint8_t disp_force_refresh;

void disp_enable_update(void);
void disp_disable_update(void);
void disp_ex_flush_ready(void);

#ifdef __cplusplus
}
#endif
#endif
