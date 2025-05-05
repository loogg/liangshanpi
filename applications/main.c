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

#ifdef BSP_USING_NES

#ifdef BSP_USING_NES_C
#include "nes.h"

static void nes_thread_entry(void* parameter) {
    const char* nes_file_path = (const char*)parameter;

    nes_t* nes = nes_init();

    do {
        int ret = nes_load_file(nes, nes_file_path);
        if (ret) {
            NES_LOG_ERROR("nes load file fail\n");
            break;
        }

        nes_run(nes);
        nes_unload_file(nes);
    } while (0);

    nes_deinit(nes);
}
#endif /* BSP_USING_NES_C */

#ifdef BSP_USING_NES_OPENEDV
#include "nes_main.h"

static void nes_thread_entry(void* parameter) {
    const char* nes_file_path = (const char*)parameter;

    nes_load((uint8_t *)nes_file_path);
}
#endif

static int nes_start(int argc, char* argv[]) {
    static char nes_file_path[256];
    if (argc == 2) {
        snprintf(nes_file_path, sizeof(nes_file_path), "%s", argv[1]);
        size_t nes_file_path_len = strlen(nes_file_path);
        if (memcmp(nes_file_path+nes_file_path_len-4,".nes",4)==0 || memcmp(nes_file_path+nes_file_path_len-4,".NES",4)==0){
            rt_kprintf("nes_file_path:%s\n",nes_file_path);
            rt_thread_t tid = rt_thread_create("nes", nes_thread_entry, nes_file_path, 4096, 21, 10);
            if (tid == RT_NULL) {
                rt_kprintf("Can't create nes thread!\n");
                return -1;
            }
            rt_thread_startup(tid);
            return 0;
        } else {
            rt_kprintf("Please enter xxx.nes\n");
            return -1;
        }
    } else {
        rt_kprintf("Please enter the nes file path\n");
        return -1;
    }
}
MSH_CMD_EXPORT(nes_start, start nes emulator);

#endif /* BSP_USING_NES */
