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

lv_ui guider_ui;

void lv_user_gui_init(void)
{
    setup_ui(&guider_ui);
    custom_init(&guider_ui);
}
