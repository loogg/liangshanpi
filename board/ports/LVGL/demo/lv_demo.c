/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author        Notes
 * 2021-10-17     Meco Man      first version
 * 2022-05-10     Meco Man      improve rt-thread initialization process
 */
#include <lvgl.h>
#include "gui_guider.h"
#include "custom.h"

#define DBG_TAG "lvgl.pro"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

extern int sdcard_check_mount(void);

lv_ui guider_ui;
lv_font_t *lv_font_sc_regular_14 = NULL;

void lv_user_gui_init(void)
{
    while(!sdcard_check_mount())
    {
        rt_thread_mdelay(200);
    }

    setup_ui(&guider_ui);
    custom_init(&guider_ui);
}
