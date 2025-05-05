#ifndef __NESPLAYER_H
#define __NESPLAYER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "nes.h"

int nesplayer_play(char *uri);
int nesplayer_stop(void);
int nesplayer_send_key_event(uint32_t key_event);

#ifdef __cplusplus
}
#endif
#endif
