/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-10-18     Meco Man     The first version
 */
#include <lvgl.h>
#include <stdbool.h>
#include <rtdevice.h>
#include <board.h>
#include <drv_lcd.h>
#include "usbh_standard_hid.h"
#include "nesplayer.h"

lv_indev_t *touch_indev;
lv_indev_t *enc_indev;

static lv_indev_state_t last_state = LV_INDEV_STATE_REL;
static rt_int16_t last_x = 0;
static rt_int16_t last_y = 0;

static int16_t enc_diff = 0;
static lv_indev_state_t enc_state = LV_INDEV_STATE_REL;

static lv_indev_state_t last_right_state = LV_INDEV_STATE_REL;

static void input_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
{
    data->point.x = last_x;
    data->point.y = last_y;
    data->state = last_state;
}

static void mousewheel_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{
    (void) indev_drv;      /*Unused*/

    data->state = enc_state;
    data->enc_diff = enc_diff;
    enc_diff = 0;
}

void usr_mouse_process_data (mouse_report_data *data) {
    // if ((0U != data->x) && (0U != data->y)) {
    //     last_x = data->x;
    //     last_y = data->y;
    //     rt_kprintf("> X = %d, Y = %d.\r\n", data->x, data->y);
    // }
    last_x += data->x;
    last_y += data->y;
    if (last_x < 0) last_x = 0;
    if (last_x > LCD_W) last_x = LCD_W;
    if (last_y < 0) last_y = 0;
    if (last_y > LCD_H) last_y = LCD_H;
    last_state = data->buttons[0] ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;

    enc_diff = data->wheel;
    enc_state = data->buttons[2] ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;

    if (last_right_state == LV_INDEV_STATE_PR && data->buttons[1] == 0) {
        nesplayer_stop();
    }

    last_right_state = data->buttons[1] ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
}

void lv_port_indev_init(void)
{
    static lv_indev_drv_t indev_drv;

    lv_indev_drv_init(&indev_drv); /*Basic initialization*/
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = input_read;

    /*Register the driver in LVGL and save the created input device object*/
    touch_indev = lv_indev_drv_register(&indev_drv);

    /*Set a cursor for the mouse*/
  LV_IMG_DECLARE(mouse_cursor_icon); /*Declare the image file.*/
  lv_obj_t * cursor_obj = lv_img_create(lv_scr_act()); /*Create an image object for the cursor */
  lv_img_set_src(cursor_obj, &mouse_cursor_icon);           /*Set the image source*/
  lv_indev_set_cursor(touch_indev, cursor_obj);             /*Connect the image  object to the driver*/

  static lv_indev_drv_t indev_drv_3;
    lv_indev_drv_init(&indev_drv_3); /*Basic initialization*/
    indev_drv_3.type = LV_INDEV_TYPE_ENCODER;
    indev_drv_3.read_cb = mousewheel_read;
    enc_indev = lv_indev_drv_register(&indev_drv_3);

    lv_group_t *group = lv_group_create();
    lv_group_set_default(group);
    lv_indev_set_group(enc_indev, group);
    lv_indev_set_group(touch_indev, group);
}
