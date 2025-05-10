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
#include <dfs_file.h>
#include <poll.h>
#include <unistd.h>

#define DBG_TAG "lvgl.pro"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

#define SD_TTF_PATH "sdio/素材集市康康体.ttf"
#define TMP_TTF_PATH "tmp/素材集市康康体.ttf"
#define FREETYPE_TTF_PATH "S:" TMP_TTF_PATH

extern int sdcard_check_mount(void);

lv_ui guider_ui;
lv_font_t *lv_font_cubic_24 = NULL;

void lv_user_gui_init(void)
{
    void copy(const char *src, const char *dst);

    while(!sdcard_check_mount())
    {
        rt_thread_mdelay(200);
    }

    // struct stat s;
    // if (stat(SD_TTF_PATH, &s) != 0) {
    //     LOG_E("TTF file not found");
    //     return;
    // }

    // uint8_t *buf = rt_malloc(s.st_size);
    // if (buf == NULL) {
    //     LOG_E("Failed to allocate memory for TTF file");
    //     return;
    // }
    // int fd = open(SD_TTF_PATH, O_RDONLY);
    // if (fd < 0) {
    //     LOG_E("Failed to open TTF file");
    //     rt_free(buf);
    //     return;
    // }
    // if (read(fd, buf, s.st_size) != s.st_size) {
    //     LOG_E("Failed to read TTF file");
    //     close(fd);
    //     rt_free(buf);
    //     return;
    // }
    // close(fd);

    // lv_font_cubic_24 = lv_tiny_ttf_create_data(buf, s.st_size, 24);
    // if (lv_font_cubic_24 == NULL) {
    //     LOG_E("Failed to create font from TTF data");
    //     rt_free(buf);
    //     return;
    // }

    copy(SD_TTF_PATH, TMP_TTF_PATH);
    /*Create a font*/
    static lv_ft_info_t info;
    /*FreeType uses C standard file system, so no driver letter is required.*/
    info.name = FREETYPE_TTF_PATH;
    info.weight = 24;
    info.style = FT_FONT_STYLE_NORMAL;
    info.mem = NULL;
    if(!lv_ft_font_init(&info)) {
        LOG_E("create failed.");
        return;
    }

    lv_font_cubic_24 = info.font;

    setup_ui(&guider_ui);
    custom_init(&guider_ui);
}
