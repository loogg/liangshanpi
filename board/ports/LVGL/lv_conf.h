/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author        Notes
 * 2021-10-18     Meco Man      First version
 */

#ifndef LV_CONF_H
#define LV_CONF_H

#include <rtconfig.h>

#define LV_COLOR_DEPTH          16
#define LV_COLOR_16_SWAP        1
#define LV_USE_PERF_MONITOR     1
#define LV_HOR_RES_MAX          240
#define LV_VER_RES_MAX          280

#define LV_USE_ARC        0
#define LV_USE_CANVAS     0
#define LV_USE_DROPDOWN   0
// #define LV_USE_IMG        0
#define LV_USE_LINE       0
#define LV_USE_ROLLER     0
#define LV_USE_SLIDER     0
#define LV_USE_TABLE      0
#define LV_USE_ANIMIMG    0
#define LV_USE_CALENDAR   0
#define LV_USE_CHART      0
#define LV_USE_COLORWHEEL 0
#define LV_USE_IMGBTN     0
#define LV_USE_KEYBOARD   0
#define LV_USE_LED        0
#define LV_USE_LIST       0
#define LV_USE_MENU       0
#define LV_USE_METER      0
#define LV_USE_MSGBOX     0
#define LV_USE_SPAN       0
#define LV_USE_SPINNER    0
#define LV_USE_TABVIEW    0
#define LV_USE_TILEVIEW   0
#define LV_USE_WIN        0

#define LV_USE_FS_STDIO        1
#define LV_FS_STDIO_LETTER     'S'
#define LV_FS_STDIO_CACHE_SIZE 4096

#define LV_USE_GIF 1

// #define LV_USE_DEMO_WIDGETS 1

// #ifdef PKG_USING_LV_MUSIC_DEMO
// /* music player demo */
// #define LV_USE_DEMO_RTT_MUSIC       1
// #define LV_DEMO_RTT_MUSIC_AUTO_PLAY 1
// #define LV_FONT_MONTSERRAT_12       1
// #define LV_FONT_MONTSERRAT_16       1
// #define LV_COLOR_SCREEN_TRANSP      1
// #endif /* PKG_USING_LV_MUSIC_DEMO */

#endif
