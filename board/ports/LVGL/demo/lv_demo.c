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
#include "lv_tiny_ttf.h"
#include <dfs_file.h>
#include <poll.h>
#include <unistd.h>

#define DBG_TAG "lvgl.pro"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

#define SD_TTF_PATH "sdio/Cubic_11.ttf"
#define TMP_TTF_PATH "tmp/Cubic_11.ttf"
#define FREETYPE_TTF_PATH "S:" TMP_TTF_PATH

extern int sdcard_check_mount(void);

lv_ui guider_ui;
lv_font_t *lv_ttf_font_32 = NULL;
lv_font_t *lv_ttf_font_30 = NULL;
lv_font_t *lv_ttf_font_28 = NULL;
lv_font_t *lv_ttf_font_26 = NULL;
lv_font_t *lv_ttf_font_24 = NULL;
lv_font_t *lv_ttf_font_22 = NULL;
lv_font_t *lv_ttf_font_20 = NULL;
lv_font_t *lv_ttf_font_18 = NULL;
lv_font_t *lv_ttf_font_16 = NULL;
lv_font_t *lv_ttf_font_14 = NULL;
lv_font_t *lv_ttf_font_12 = NULL;
lv_font_t *lv_ttf_font_10 = NULL;
lv_font_t *lv_ttf_font_8 = NULL;


#ifdef BSP_LVGL_DEMO_USING_FREETYPE
lv_ft_info_t ft_info;
#endif

void lv_user_gui_init(void)
{
    while(!sdcard_check_mount())
    {
        rt_thread_mdelay(200);
    }

    void copy(const char *src, const char *dst);
    copy(SD_TTF_PATH, TMP_TTF_PATH);

#ifdef BSP_LVGL_DEMO_USING_TINYTTF
    lv_ttf_font_32 = lv_tiny_ttf_create_file_ex(FREETYPE_TTF_PATH, 32, 32*1024);
    lv_ttf_font_30 = lv_tiny_ttf_create_file_ex(FREETYPE_TTF_PATH, 30, 32*1024);
    lv_ttf_font_28 = lv_tiny_ttf_create_file_ex(FREETYPE_TTF_PATH, 28, 32*1024);
    lv_ttf_font_26 = lv_tiny_ttf_create_file_ex(FREETYPE_TTF_PATH, 26, 32*1024);
    lv_ttf_font_24 = lv_tiny_ttf_create_file_ex(FREETYPE_TTF_PATH, 24, 32*1024);
    lv_ttf_font_22 = lv_tiny_ttf_create_file_ex(FREETYPE_TTF_PATH, 22, 32*1024);
    lv_ttf_font_20 = lv_tiny_ttf_create_file_ex(FREETYPE_TTF_PATH, 20, 32*1024);
    lv_ttf_font_18 = lv_tiny_ttf_create_file_ex(FREETYPE_TTF_PATH, 18, 32*1024);
    lv_ttf_font_16 = lv_tiny_ttf_create_file_ex(FREETYPE_TTF_PATH, 16, 32*1024);
    lv_ttf_font_14 = lv_tiny_ttf_create_file_ex(FREETYPE_TTF_PATH, 14, 32*1024);
    lv_ttf_font_12 = lv_tiny_ttf_create_file_ex(FREETYPE_TTF_PATH, 12, 32*1024);
    lv_ttf_font_10 = lv_tiny_ttf_create_file_ex(FREETYPE_TTF_PATH, 10, 32*1024);
    lv_ttf_font_8 = lv_tiny_ttf_create_file_ex(FREETYPE_TTF_PATH, 8, 32*1024);
#endif

#ifdef BSP_LVGL_DEMO_USING_FREETYPE
    /*FreeType uses C standard file system, so no driver letter is required.*/
    ft_info.name = FREETYPE_TTF_PATH;
    ft_info.weight = 32;
    ft_info.style = FT_FONT_STYLE_NORMAL;
    ft_info.mem = NULL;
    if(!lv_ft_font_init(&ft_info)) {
        LOG_E("create failed.");
        return;
    }
    lv_ttf_font_32 = ft_info.font;

    ft_info.weight = 30;
    if(!lv_ft_font_init(&ft_info)) {
        LOG_E("create failed.");
        return;
    }
    lv_ttf_font_30 = ft_info.font;

    ft_info.weight = 28;
    if(!lv_ft_font_init(&ft_info)) {
        LOG_E("create failed.");
        return;
    }
    lv_ttf_font_28 = ft_info.font;

    ft_info.weight = 26;
    if(!lv_ft_font_init(&ft_info)) {
        LOG_E("create failed.");
        return;
    }
    lv_ttf_font_26 = ft_info.font;

    ft_info.weight = 24;
    if(!lv_ft_font_init(&ft_info)) {
        LOG_E("create failed.");
        return;
    }
    lv_ttf_font_24 = ft_info.font;

    ft_info.weight = 22;
    if(!lv_ft_font_init(&ft_info)) {
        LOG_E("create failed.");
        return;
    }
    lv_ttf_font_22 = ft_info.font;

    ft_info.weight = 20;
    if(!lv_ft_font_init(&ft_info)) {
        LOG_E("create failed.");
        return;
    }
    lv_ttf_font_20 = ft_info.font;

    ft_info.weight = 18;
    if(!lv_ft_font_init(&ft_info)) {
        LOG_E("create failed.");
        return;
    }
    lv_ttf_font_18 = ft_info.font;

    ft_info.weight = 16;
    if(!lv_ft_font_init(&ft_info)) {
        LOG_E("create failed.");
        return;
    }
    lv_ttf_font_16 = ft_info.font;

    ft_info.weight = 14;
    if(!lv_ft_font_init(&ft_info)) {
        LOG_E("create failed.");
        return;
    }
    lv_ttf_font_14 = ft_info.font;

    ft_info.weight = 12;
    if(!lv_ft_font_init(&ft_info)) {
        LOG_E("create failed.");
        return;
    }
    lv_ttf_font_12 = ft_info.font;

    ft_info.weight = 10;
    if(!lv_ft_font_init(&ft_info)) {
        LOG_E("create failed.");
        return;
    }
    lv_ttf_font_10 = ft_info.font;

    ft_info.weight = 8;
    if(!lv_ft_font_init(&ft_info)) {
        LOG_E("create failed.");
        return;
    }
    lv_ttf_font_8 = ft_info.font;
#endif

    setup_ui(&guider_ui);
    custom_init(&guider_ui);
}
