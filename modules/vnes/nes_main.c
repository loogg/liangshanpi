/**
 ****************************************************************************************************
 * @file        nes_main.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.1
 * @date        2022-05-31
 * @brief       NES主函数 代码
 *              本程序移植自网友ye781205的NES模拟器工程, 特此感谢!
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 * @attention
 *
 * 实验平台:正点原子 开发板
 * 在线视频:www.yuanzige.com
 * 技术论坛:www.openedv.com
 * 公司网址:www.alientek.com
 * 购买地址:openedv.taobao.com
 *
 * 修改说明
 * V1.1 20220531
 * 1, 修改注释方式
 * 2, 修改uint8_t/uint16_t/uint32_t为uint8_t/uint16_t/uint32_t
 ****************************************************************************************************
 */

#include "nes_main.h"
#include "nes_ppu.h"
#include "nes_mapper.h"
#include "nes_apu.h"
#include "string.h"
#include <stdio.h>

#include <dfs_file.h>
#include <poll.h>
#include <unistd.h>

#include <drv_lcd.h>

#define DBG_TAG    "vnes.main"
#define DBG_LVL    DBG_INFO
#include <rtdbg.h>

volatile uint8_t system_task_return; /* 任务强制返回标志 */
volatile uint8_t g_framecnt;   /* nes帧计数器 */

int MapperNo;           /* map编号 */
int NES_scanline;       /* nes扫描线 */
int VROM_1K_SIZE;
int VROM_8K_SIZE;
uint32_t NESrom_crc32;

uint8_t PADdata0;       /* 手柄1键值 [7:0]右7 左6 下5 上4 Start3 Select2 B1 A0 */
uint8_t PADdata1;       /* 手柄2键值 [7:0]右7 左6 下5 上4 Start3 Select2 B1 A0 */
uint8_t *NES_RAM;       /* 保持1024字节对齐 */
uint8_t *NES_SRAM;
NES_header *RomHeader;  /* rom文件头 */
MAPPER *NES_Mapper;
MapperCommRes *MAPx;


uint8_t *spr_ram;       /* 精灵RAM,256字节 */
ppu_data *ppu;          /* ppu指针 */
uint8_t *VROM_banks;
uint8_t vrom_banks_alloc_flag = 0;
uint8_t *VROM_tiles;

apu_t *apu;             /* apu指针 */
uint16_t *wave_buffers;

uint8_t *romfile;       /* nes文件指针,指向整个nes文件的起始地址 */

#define SOUND_DEVICE_NAME   "sound0"

static rt_device_t _sound_device = RT_NULL;

/**
 * @brief       加载ROM
 * @param       无
 * @retval      0, 成功;
 *              1, 内存错误
 *              3, map错误
 */
uint8_t nes_load_rom(void)
{
    uint8_t* p;
    uint8_t i;
    uint8_t res = 0;
    p = (uint8_t*)romfile;

    if (strncmp((char*)p,"NES",3) == 0)
    {
        RomHeader->ctrl_z = p[3];
        RomHeader->num_16k_rom_banks = p[4];
        RomHeader->num_8k_vrom_banks = p[5];
        RomHeader->flags_1 = p[6];
        RomHeader->flags_2 = p[7];
        if (RomHeader->flags_1&0x04) p+= 512;   /* 有512字节的trainer: */

        if (RomHeader->num_8k_vrom_banks > 0)   /* 存在VROM,进行预解码 */
        {
            vrom_banks_alloc_flag = 0;
            VROM_banks = p + 16 + (RomHeader->num_16k_rom_banks * 0x4000);
#if	NES_RAM_SPEED == 1    /* 1:内存占用小 0:速度快 */
            VROM_tiles=VROM_banks;
#else

            VROM_tiles = mymalloc(SRAMEX,RomHeader->num_8k_vrom_banks*8*1024);  /* 这里可能申请多达1MB内存!!! */
            if (VROM_tiles == 0) VROM_tiles = VROM_banks;           /* 内存不够用的情况下,尝试VROM_titles与VROM_banks共用内存 */

            compile(RomHeader->num_8k_vrom_banks * 8 * 1024 / 16,VROM_banks,VROM_tiles);
#endif
        }
        else
        {
            vrom_banks_alloc_flag = 1;
            VROM_banks = mymalloc(SRAMIN,8 * 1024);
            VROM_tiles = mymalloc(SRAMEX,8 * 1024);

            if (!VROM_banks || !VROM_tiles) res = 1;
        }

        VROM_1K_SIZE = RomHeader->num_8k_vrom_banks * 8;
        VROM_8K_SIZE = RomHeader->num_8k_vrom_banks;
        MapperNo = (RomHeader->flags_1 >> 4) | (RomHeader->flags_2 & 0xf0);

        if (RomHeader->flags_2 & 0x0E) MapperNo = RomHeader->flags_1 >> 4;    /* 忽略高四位，如果头看起来很糟糕  */

        printf("use map:%d\r\n",MapperNo);

        for (i = 0;i < 255;i ++)  /*  查找支持的Mapper号 */
        {
            if (MapTab[i] == MapperNo) break;
            if (MapTab[i] == -1) res = 3;
        }

        if (res == 0)
        {
            switch (MapperNo)
            {
                case 1:
                    MAP1 = mymalloc(SRAMIN,sizeof(Mapper1Res));
                    if (!MAP1) res = 1;
                    break;
                case 4:
                case 6:
                case 16:
                case 17:
                case 18:
                case 19:
                case 21:
                case 23:
                case 24:
                case 25:
                case 64:
                case 65:
                case 67:
                case 69:
                case 85:
                case 189:
                    MAPx = mymalloc(SRAMIN,sizeof(MapperCommRes));
                    if (!MAPx) res = 1;
                    break;
                default:
                    break;
            }
        }
    }
    return res; /* 返回执行结果 */
}

/**
 * @brief       释放内存
 * @param       无
 * @retval      无
 */
void nes_sram_free(void)
{
    rt_free_align(NES_RAM);
    myfree(SRAMIN,NES_SRAM);
    myfree(SRAMIN,RomHeader);
    myfree(SRAMIN,NES_Mapper);
    myfree(SRAMIN,spr_ram);
    myfree(SRAMIN,ppu);
    myfree(SRAMIN,apu);
    myfree(SRAMIN,wave_buffers);
    myfree(SRAMEX,romfile);

    if ((VROM_tiles != VROM_banks) && VROM_banks && VROM_tiles)/* 如果分别为VROM_banks和VROM_tiles申请了内存,则释放 */
    {
        if (vrom_banks_alloc_flag) {
            myfree(SRAMIN,VROM_banks);
        }
        myfree(SRAMEX,VROM_tiles);
    }

    switch (MapperNo)   /* 释放map内存 */
    {
        case 1: /* 释放内存 */
            myfree(SRAMIN,MAP1);
            break;
        case 4:
        case 6:
        case 16:
        case 17:
        case 18:
        case 19:
        case 21:
        case 23:
        case 24:
        case 25:
        case 64:
        case 65:
        case 67:
        case 69:
        case 85:
        case 189:
            myfree(SRAMIN,MAPx);break;  /* 释放内存 */
        default:break;
    }

    NES_RAM = 0;
    NES_SRAM = 0;
    RomHeader = 0;
    NES_Mapper = 0;
    spr_ram = 0;
    ppu = 0;
    apu = 0;
    wave_buffers = 0;
    romfile = 0;
    VROM_banks = 0;
    vrom_banks_alloc_flag = 0;
    VROM_tiles = 0;
    MAP1 = 0;
    MAPx = 0;
}

/**
 * @brief       为NES运行申请内存
 * @param       mbuf            : nes文件大小
 * @retval      0, 成功;  其他, 失败;
 */
uint8_t nes_sram_malloc(uint32_t romsize)
{
    uint16_t i = 0;
    for (i = 0;i < 64;i ++)                     /* 为NES_RAM,查找1024对齐的内存 */
    {
        NES_RAM = rt_malloc_align(0X800,1024); /* 申请1024字节对齐的内存 */

        if ((uint32_t) NES_RAM % 1024)          /* 不是1024字节对齐 */
        {
            myfree(SRAMEX,NES_RAM);             /* 释放内存,然后重新尝试分配 */
        }
        else
        {
            break;
        }
    }

    NES_SRAM = mymalloc(SRAMIN,0X2000);
    RomHeader = mymalloc(SRAMIN,sizeof(NES_header));
    NES_Mapper = mymalloc(SRAMIN,sizeof(MAPPER));
    spr_ram = mymalloc(SRAMIN,0X100);
    ppu = mymalloc(SRAMIN,sizeof(ppu_data));
    apu = mymalloc(SRAMIN,sizeof(apu_t));       /* sizeof(apu_t)=  12588 */
    wave_buffers = mymalloc(SRAMIN,APU_PCMBUF_SIZE * 2);
    romfile = mymalloc(SRAMEX,romsize);         /* 申请游戏rom空间,等于nes文件大小 */

    if (romfile == NULL)                        /* 内存不够?释放主界面占用内存,再重新申请 */
    {
        romfile = mymalloc(SRAMEX,romsize);     /* 重新申请 */
    }

    if (i == 64 || !NES_RAM || !NES_SRAM || !RomHeader || !NES_Mapper || !spr_ram || !ppu || !apu || !wave_buffers || !romfile)
    {
        nes_sram_free();
        return 1;
    }

    memset(NES_SRAM,0,0X2000);              /* 清零 */
    memset(RomHeader,0,sizeof(NES_header)); /* 清零 */
    memset(NES_Mapper,0,sizeof(MAPPER));    /* 清零 */
    memset(spr_ram,0,0X100);                /* 清零 */
    memset(ppu,0,sizeof(ppu_data));         /* 清零 */
    memset(apu,0,sizeof(apu_t));            /* 清零 */
    memset(wave_buffers,0,APU_PCMBUF_SIZE * 2); /* 清零 */
    memset(romfile,0,romsize);                  /* 清零 */
    return 0;
}

/**
 * @brief       开始nes游戏
 * @param       pname           : nes游戏路径
 * @retval      0, 正常退出
 *              1, 内存错误
 *              2, 文件错误
 *              3, map错误
 */
uint8_t nes_load(uint8_t* pname)
{
    struct stat s;
    uint8_t *buf;       /* 缓存 */
    uint8_t *p;
    uint32_t readlen;   /* 总读取长度 */
    int bread;     /* 读取的长度 */

    uint8_t res = 0;

    buf = mymalloc(SRAMIN, 1024);

    if (stat((char *)pname, &s) != 0) /* 获取文件大小 */
    {
        myfree(SRAMIN, buf);
        return 2;
    }

    int fd = open((char *)pname, O_RDONLY); /* 打开文件 */
    if (fd < 0)   /* 打开文件失败 */
    {
        myfree(SRAMIN, buf);
        return 2;
    }

    res = nes_sram_malloc(s.st_size);   /* 申请内存 */

    if (res == 0)
    {
        p = romfile;
        readlen = 0;

        while (readlen < s.st_size)
        {
            bread = read(fd, buf, 1024); /* 读取文件内容 */
            readlen += bread;
            my_mem_copy(p, buf, bread);
            p += bread;

            if (bread <= 0)break;
        }

        NESrom_crc32 = get_crc32(romfile + 16, s.st_size - 16); /* 获取CRC32的值 */
        res = nes_load_rom();                       /* 加载ROM */

        if (res == 0)
        {
            cpu6502_init();                         /* 初始化6502,并复位 */
            Mapper_Init();                          /* map初始化 */
            PPU_reset();                            /* ppu复位 */
            apu_init();                             /* apu初始化  */
            nes_sound_open(0,APU_SAMPLE_RATE);      /* 初始化播放设备 */
            system_task_return = 0;
            nes_emulate_frame();                    /* 进入NES模拟器主循环 */
            nes_sound_close();                      /* 关闭声音输出 */
        }
    }

    close(fd);                               /* 关闭文件 */
    myfree(SRAMIN, buf);    /* 释放内存 */
    nes_sram_free();        /* 释放内存 */
    return res;
}

uint16_t nes_xoff = 0;                              /* 显示在x轴方向的偏移量(实际显示宽度=256-2*nes_xoff) */
uint16_t nes_yoff = 0;                              /* 显示在y轴方向的偏移量 */

uint16_t *lcd_frame_ptr = NULL;
uint32_t lcd_frame_write_index = 0; /* LCD显示数据写入索引 */

int nes_get_framebuffer(void);
int nes_frame_draw(void);


/**
 * @brief       设置游戏显示窗口
 * @param       无
 * @retval      无
 */
void nes_set_window(void)
{
    uint16_t xoff = 0,yoff = 0;
    uint16_t lcdwidth,lcdheight;

    lcdwidth = 240;
    lcdheight = 240;
    nes_xoff = (256 - lcdwidth) / 2;    /* 得到x轴方向的偏移量 */

    // lcd_address_set(0, 0, lcdwidth - 1, lcdheight - 1);
}


/**
 * @brief       nes模拟器主循环
 * @param       无
 * @retval      无
 */
void nes_emulate_frame(void)
{
    uint8_t nes_frame;
    nes_set_window();               /* 设置窗口 */

    while (1)
    {
        rt_thread_mdelay(2);

        /*  LINES 0-239 */
        PPU_start_frame();

        if (nes_frame == 0) {
            nes_get_framebuffer(); /* 获取LCD显示缓存 */
        }

        rt_tick_t t1 = rt_tick_get();
        for (NES_scanline = 0; NES_scanline < 240; NES_scanline++)
        {
            run6502(113*256);
            NES_Mapper->HSync(NES_scanline);
            /* 扫描一行 */
            if (nes_frame == 0) scanline_draw(NES_scanline);
            else do_scanline_and_dont_draw(NES_scanline);
        }
        rt_tick_t t2 = rt_tick_get();
        rt_tick_t t_diff = t2 - t1;
        // rt_kprintf("nes frame time:%u\r\n", t_diff);

        if (nes_frame == 0) {
            nes_frame_draw(); /* 刷新LCD显示缓存 */
        }

        NES_scanline = 240;
        run6502(113 * 256);/* 运行1线 */
        NES_Mapper->HSync(NES_scanline);
        start_vblank();

        if (NMI_enabled())
        {
            cpunmi=1;
            run6502(7*256);/* 运行中断 */
        }

        NES_Mapper->VSync();
        /*  LINES 242-261 */
        for (NES_scanline = 241;NES_scanline < 262;NES_scanline ++)
        {
            run6502(113*256);
            NES_Mapper->HSync(NES_scanline);
        }

        end_vblank();
        nes_get_gamepadval();   /* 每3帧查询一次USB */
        apu_soundoutput();  /* 输出游戏声音 */
        g_framecnt++;
        nes_frame++;

        if (nes_frame > NES_SKIP_FRAME) nes_frame = 0;/* 跳帧 */

        if (system_task_return)
        {
            break;
        }

        nes_set_window();               /* 设置窗口 */
    }
}

/**
 * @brief       6502调试输出
 *  @note       在6502.s里面被调用
 * @param       reg0            : 寄存器0
 * @param       reg1            : 寄存器1
 * @retval      无
 */
void debug_6502(uint16_t reg0,uint8_t reg1)
{
    printf("6502 error:%x,%d\r\n",reg0,reg1);
}


/**
 * @brief       NES打开音频输出
 * @param       samples_per_sync: 未用到
 * @param       sample_rate     : 音频采样率
 * @retval      无
 */
int nes_sound_open(int samples_per_sync,int sample_rate)
{
    printf("sound open:%d\r\n",sample_rate);

    rt_err_t result = RT_EOK;
    struct rt_audio_caps caps;

    _sound_device = rt_device_find(SOUND_DEVICE_NAME);
    if (_sound_device == RT_NULL) {
        LOG_E("sound device %s not find", SOUND_DEVICE_NAME);
        return -RT_ERROR;
    }

    result = rt_device_open(_sound_device, RT_DEVICE_OFLAG_WRONLY);
    if (result != RT_EOK) {
        LOG_E("open %s device failed", SOUND_DEVICE_NAME);
        rt_device_close(_sound_device);
        return -RT_ERROR;
    }

    /* set sampletate,channels, samplebits */
    caps.main_type = AUDIO_TYPE_OUTPUT;
    caps.sub_type  = AUDIO_DSP_PARAM;
    caps.udata.config.samplerate = sample_rate;
    caps.udata.config.channels = 1;
    caps.udata.config.samplebits = 16;
    rt_device_control(_sound_device, AUDIO_CTL_CONFIGURE, &caps);
    return 1;
}

/**
 * @brief       NES关闭音频输出
 * @param       无
 * @retval      无
 */
void nes_sound_close(void)
{
    if (_sound_device != RT_NULL) {
        rt_device_close(_sound_device);
        _sound_device = RT_NULL;
    }
}

/**
 * @brief       NES音频输出到SAI缓存
 * @param       无
 * @retval      无
 */
void nes_apu_fill_buffer(int samples,uint16_t* wavebuf)
{
    rt_device_write(_sound_device, 0, wavebuf, APU_PCMBUF_SIZE * 2); /* 发送数据到音频设备 */
}
