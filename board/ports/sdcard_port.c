/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-12-13     balanceTWK   add sdcard port file
 * 2021-02-18     DavidLin     Fixed the return bug
 */

#include <rtthread.h>

#ifdef BSP_USING_SDCARD

#include <dfs_elm.h>
#include <dfs_fs.h>
#include <dfs_file.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/statfs.h>

#define DBG_TAG "app.card"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

static uint8_t _mounted = 0;

void sd_mount(void *parameter)
{
    while (1)
    {
        rt_thread_mdelay(500);
        if(rt_device_find("sd0") != RT_NULL)
        {
            if (dfs_mount("sd0", "/sdio", "elm", 0, 0) == RT_EOK)
            {
                LOG_I("sd card mount to '/sdio'");
                _mounted = 1;
                break;
            }
            else
            {
                LOG_W("sd card mount to '/sdio' failed!");
            }
        }
    }

    while (1)
    {
        rt_thread_mdelay(1000);
    }

}

int sdcard_check_mount(void) {
    return _mounted;
}

int stm32_sdcard_mount(void)
{
    rt_thread_t tid;

    tid = rt_thread_create("sd_mount", sd_mount, RT_NULL,
                           2048, RT_THREAD_PRIORITY_MAX - 2, 20);
    if (tid != RT_NULL)
    {
        rt_thread_startup(tid);
    }
    else
    {
        LOG_E("create sd_mount thread err!");
        return -RT_ERROR;
    }
    return RT_EOK;
}
// INIT_APP_EXPORT(stm32_sdcard_mount);

#endif /* BSP_USING_SDCARD */

