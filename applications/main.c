/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-11-06     SummerGift   first version
 * 2023-12-03     Meco Man     support nano version
 */

#include <board.h>
#include <rtthread.h>
#include <drv_gpio.h>
#ifndef RT_USING_NANO
#include <rtdevice.h>
#endif /* RT_USING_NANO */

#include <dfs_romfs.h>
#include <dfs_fs.h>
#include <dfs_file.h>
#include <poll.h>
#include <unistd.h>

#define DBG_TAG    "main"
#define DBG_LVL    DBG_INFO
#include <rtdbg.h>

#define LED0_PIN    GET_PIN(E, 3)

extern const struct romfs_dirent romfs_root;

extern void usb_demo_init(void);
extern int stm32_sdcard_mount(void);

int main(void)
{
    /* set LED0 pin mode to output */
    rt_pin_mode(LED0_PIN, PIN_MODE_OUTPUT);

    dfs_mount(RT_NULL, "/", "rom", 0, &(romfs_root));

    stm32_sdcard_mount();
    usb_demo_init();

    while (1)
    {
        rt_pin_write(LED0_PIN, PIN_HIGH);
        rt_thread_mdelay(500);
        rt_pin_write(LED0_PIN, PIN_LOW);
        rt_thread_mdelay(500);
    }
}

void show_all_clk(void) {
    rt_kprintf("SystemCoreClock:%d\n", SystemCoreClock);
    rt_kprintf("HAL_RCC_GetSysClockFreq:%d\n", HAL_RCC_GetSysClockFreq());
    rt_kprintf("HAL_RCC_GetHCLKFreq:%d\n", HAL_RCC_GetHCLKFreq());
    rt_kprintf("HAL_RCC_GetPCLK1Freq:%d\n", HAL_RCC_GetPCLK1Freq());
    rt_kprintf("HAL_RCC_GetPCLK2Freq:%d\n", HAL_RCC_GetPCLK2Freq());
}
MSH_CMD_EXPORT(show_all_clk, show all clock frequency);
