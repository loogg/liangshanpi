/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-10-18     Meco Man     The first version
 */
#include <lvgl.h>
#include <drv_lcd.h>

#define MY_DISP_HOR_RES     LCD_W
#define DISP_BUFFER_LINES   (LCD_H/2)

/*A static or global variable to store the buffers*/
static lv_disp_draw_buf_t disp_buf;

/*Descriptor of a display driver*/
static lv_disp_drv_t disp_drv;

/*Static or global buffer(s). The second buffer is optional*/
static lv_color_t buf_1[MY_DISP_HOR_RES * DISP_BUFFER_LINES];
static lv_color_t buf_2[MY_DISP_HOR_RES * DISP_BUFFER_LINES];

static volatile bool disp_flush_enabled = true;
uint8_t disp_force_refresh = 0;

/* Enable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_enable_update(void)
{
    disp_flush_enabled = true;
    disp_force_refresh = 1;
}

/* Disable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_disable_update(void)
{
    disp_flush_enabled = false;
}

void disp_ex_flush_ready(void) {
    lv_disp_flush_ready(&disp_drv);
}

/*Flush the content of the internal buffer the specific area on the display
 *You can use DMA or any hardware acceleration to do this operation in the background but
 *'lv_disp_flush_ready()' has to be called when finished.*/
static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{
    /* color_p is a buffer pointer; the buffer is provided by LVGL */
    if (disp_flush_enabled) {
        lcd_fill_array_async(area->x1, area->y1, area->x2, area->y2, color_p);
    } else {
        lv_disp_flush_ready(disp_drv);
    }
}

void lv_port_disp_init(void)
{
    /*Initialize `disp_buf` with the buffer(s). With only one buffer use NULL instead buf_2 */
    lv_disp_draw_buf_init(&disp_buf, buf_1, buf_2, MY_DISP_HOR_RES * DISP_BUFFER_LINES);

    lv_disp_drv_init(&disp_drv); /*Basic initialization*/

    /*Set the resolution of the display*/
    disp_drv.hor_res = LCD_W;
    disp_drv.ver_res = LCD_H;

    /*Set a display buffer*/
    disp_drv.draw_buf = &disp_buf;

    /*Used to copy the buffer's content to the display*/
    disp_drv.flush_cb = disp_flush;


    /*Finally register the driver*/
    lv_disp_drv_register(&disp_drv);
}
