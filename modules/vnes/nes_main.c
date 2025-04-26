/**
 ****************************************************************************************************
 * @file        nes_main.c
 * @author      ����ԭ���Ŷ�(ALIENTEK)
 * @version     V1.1
 * @date        2022-05-31
 * @brief       NES������ ����
 *              ��������ֲ������ye781205��NESģ��������, �ش˸�л!
 * @license     Copyright (c) 2020-2032, ������������ӿƼ����޹�˾
 ****************************************************************************************************
 * @attention
 *
 * ʵ��ƽ̨:����ԭ�� ������
 * ������Ƶ:www.yuanzige.com
 * ������̳:www.openedv.com
 * ��˾��ַ:www.alientek.com
 * �����ַ:openedv.taobao.com
 *
 * �޸�˵��
 * V1.1 20220531
 * 1, �޸�ע�ͷ�ʽ
 * 2, �޸�uint8_t/uint16_t/uint32_tΪuint8_t/uint16_t/uint32_t
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

volatile uint8_t system_task_return; /* ����ǿ�Ʒ��ر�־ */
volatile uint8_t g_framecnt;   /* nes֡������ */

int MapperNo;           /* map��� */
int NES_scanline;       /* nesɨ���� */
int VROM_1K_SIZE;
int VROM_8K_SIZE;
uint32_t NESrom_crc32;

uint8_t PADdata0;       /* �ֱ�1��ֵ [7:0]��7 ��6 ��5 ��4 Start3 Select2 B1 A0 */
uint8_t PADdata1;       /* �ֱ�2��ֵ [7:0]��7 ��6 ��5 ��4 Start3 Select2 B1 A0 */
uint8_t *NES_RAM;       /* ����1024�ֽڶ��� */
uint8_t *NES_SRAM;
NES_header *RomHeader;  /* rom�ļ�ͷ */
MAPPER *NES_Mapper;
MapperCommRes *MAPx;


uint8_t *spr_ram;       /* ����RAM,256�ֽ� */
ppu_data *ppu;          /* ppuָ�� */
uint8_t *VROM_banks;
uint8_t *VROM_tiles;

apu_t *apu;             /* apuָ�� */
uint16_t *wave_buffers;

uint8_t *romfile;       /* nes�ļ�ָ��,ָ������nes�ļ�����ʼ��ַ */

#define SOUND_DEVICE_NAME   "sound0"

static rt_device_t _sound_device = RT_NULL;
static rt_device_t _lcd_device = RT_NULL;
rt_mailbox_t key_mb = RT_NULL;

/**
 * @brief       ����ROM
 * @param       ��
 * @retval      0, �ɹ�;
 *              1, �ڴ����
 *              3, map����
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
        if (RomHeader->flags_1&0x04) p+= 512;   /* ��512�ֽڵ�trainer: */

        if (RomHeader->num_8k_vrom_banks > 0)   /* ����VROM,����Ԥ���� */
        {
            VROM_banks = p + 16 + (RomHeader->num_16k_rom_banks * 0x4000);
#if	NES_RAM_SPEED == 1    /* 1:�ڴ�ռ��С 0:�ٶȿ� */
            VROM_tiles=VROM_banks;
#else

            VROM_tiles = mymalloc(SRAMEX,RomHeader->num_8k_vrom_banks*8*1024);  /* �������������1MB�ڴ�!!! */
            if (VROM_tiles == 0) VROM_tiles = VROM_banks;           /* �ڴ治���õ������,����VROM_titles��VROM_banks�����ڴ� */

            compile(RomHeader->num_8k_vrom_banks * 8 * 1024 / 16,VROM_banks,VROM_tiles);
#endif
        }
        else
        {
            VROM_banks = mymalloc(SRAMIN,8 * 1024);
            VROM_tiles = mymalloc(SRAMEX,8 * 1024);

            if (!VROM_banks || !VROM_tiles) res = 1;
        }

        VROM_1K_SIZE = RomHeader->num_8k_vrom_banks * 8;
        VROM_8K_SIZE = RomHeader->num_8k_vrom_banks;
        MapperNo = (RomHeader->flags_1 >> 4) | (RomHeader->flags_2 & 0xf0);

        if (RomHeader->flags_2 & 0x0E) MapperNo = RomHeader->flags_1 >> 4;    /* ���Ը���λ�����ͷ�����������  */

        printf("use map:%d\r\n",MapperNo);

        for (i = 0;i < 255;i ++)  /*  ����֧�ֵ�Mapper�� */
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
    return res; /* ����ִ�н�� */
}

/**
 * @brief       �ͷ��ڴ�
 * @param       ��
 * @retval      ��
 */
void nes_sram_free(void)
{
    myfree(SRAMIN,NES_RAM);
    myfree(SRAMIN,NES_SRAM);
    myfree(SRAMIN,RomHeader);
    myfree(SRAMIN,NES_Mapper);
    myfree(SRAMIN,spr_ram);
    myfree(SRAMIN,ppu);
    myfree(SRAMIN,apu);
    myfree(SRAMIN,wave_buffers);
    myfree(SRAMEX,romfile);

    if ((VROM_tiles != VROM_banks) && VROM_banks && VROM_tiles)/* ����ֱ�ΪVROM_banks��VROM_tiles�������ڴ�,���ͷ� */
    {
        myfree(SRAMIN,VROM_banks);
        myfree(SRAMEX,VROM_tiles);
    }

    switch (MapperNo)   /* �ͷ�map�ڴ� */
    {
        case 1: /* �ͷ��ڴ� */
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
            myfree(SRAMIN,MAPx);break;  /* �ͷ��ڴ� */
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
    VROM_tiles = 0;
    MAP1 = 0;
    MAPx = 0;
}

/**
 * @brief       ΪNES���������ڴ�
 * @param       mbuf            : nes�ļ���С
 * @retval      0, �ɹ�;  ����, ʧ��;
 */
uint8_t nes_sram_malloc(uint32_t romsize)
{
    uint16_t i = 0;
    for (i = 0;i < 64;i ++)                     /* ΪNES_RAM,����1024������ڴ� */
    {
        NES_RAM = rt_malloc_align(0X800,1024); /* ����1024�ֽڶ�����ڴ� */

        if ((uint32_t) NES_RAM % 1024)          /* ����1024�ֽڶ��� */
        {
            myfree(SRAMEX,NES_RAM);             /* �ͷ��ڴ�,Ȼ�����³��Է��� */
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
    romfile = mymalloc(SRAMEX,romsize);         /* ������Ϸrom�ռ�,����nes�ļ���С */

    if (romfile == NULL)                        /* �ڴ治��?�ͷ�������ռ���ڴ�,���������� */
    {
        romfile = mymalloc(SRAMEX,romsize);     /* �������� */
    }

    if (i == 64 || !NES_RAM || !NES_SRAM || !RomHeader || !NES_Mapper || !spr_ram || !ppu || !apu || !wave_buffers || !romfile)
    {
        nes_sram_free();
        return 1;
    }

    memset(NES_SRAM,0,0X2000);              /* ���� */
    memset(RomHeader,0,sizeof(NES_header)); /* ���� */
    memset(NES_Mapper,0,sizeof(MAPPER));    /* ���� */
    memset(spr_ram,0,0X100);                /* ���� */
    memset(ppu,0,sizeof(ppu_data));         /* ���� */
    memset(apu,0,sizeof(apu_t));            /* ���� */
    memset(wave_buffers,0,APU_PCMBUF_SIZE * 2); /* ���� */
    memset(romfile,0,romsize);                  /* ���� */
    return 0;
}

/**
 * @brief       ��ʼnes��Ϸ
 * @param       pname           : nes��Ϸ·��
 * @retval      0, �����˳�
 *              1, �ڴ����
 *              2, �ļ�����
 *              3, map����
 */
uint8_t nes_load(uint8_t* pname)
{
    struct stat s;
    uint8_t *buf;       /* ���� */
    uint8_t *p;
    uint32_t readlen;   /* �ܶ�ȡ���� */
    int bread;     /* ��ȡ�ĳ��� */

    uint8_t res = 0;

    if (key_mb == RT_NULL) {
        key_mb = rt_mb_create("key_mb", 32, RT_IPC_FLAG_FIFO);
    }

    buf = mymalloc(SRAMIN, 1024);

    if (stat((char *)pname, &s) != 0) /* ��ȡ�ļ���С */
    {
        myfree(SRAMIN, buf);
        return 2;
    }

    int fd = open((char *)pname, O_RDONLY); /* ���ļ� */
    if (fd < 0)   /* ���ļ�ʧ�� */
    {
        myfree(SRAMIN, buf);
        return 2;
    }

    res = nes_sram_malloc(s.st_size);   /* �����ڴ� */

    if (res == 0)
    {
        p = romfile;
        readlen = 0;

        while (readlen < s.st_size)
        {
            bread = read(fd, buf, 1024); /* ��ȡ�ļ����� */
            readlen += bread;
            my_mem_copy(p, buf, bread);
            p += bread;

            if (bread <= 0)break;
        }

        NESrom_crc32 = get_crc32(romfile + 16, s.st_size - 16); /* ��ȡCRC32��ֵ */
        res = nes_load_rom();                       /* ����ROM */

        if (res == 0)
        {
            cpu6502_init();                         /* ��ʼ��6502,����λ */
            Mapper_Init();                          /* map��ʼ�� */
            PPU_reset();                            /* ppu��λ */
            apu_init();                             /* apu��ʼ��  */
            _lcd_device = rt_device_find("lcd");
            rt_device_open(_lcd_device, RT_DEVICE_OFLAG_WRONLY);
            nes_sound_open(0,APU_SAMPLE_RATE);      /* ��ʼ�������豸 */
            nes_emulate_frame();                    /* ����NESģ������ѭ�� */
            if (_lcd_device != RT_NULL) {
                rt_device_close(_lcd_device);
                _lcd_device = RT_NULL;
            }
            nes_sound_close();                      /* �ر�������� */
        }
    }

    close(fd);                               /* �ر��ļ� */
    myfree(SRAMIN, buf);    /* �ͷ��ڴ� */
    nes_sram_free();        /* �ͷ��ڴ� */
    return res;
}

uint16_t nes_xoff = 0;                              /* ��ʾ��x�᷽���ƫ����(ʵ����ʾ���=256-2*nes_xoff) */
uint16_t nes_yoff = 0;                              /* ��ʾ��y�᷽���ƫ���� */

uint16_t *lcd_frame_ptr = NULL;
uint32_t lcd_frame_write_index = 0; /* LCD��ʾ����д������ */

static int _lcd_get_framebuffer(void) {
    struct rt_device_graphic_info info;
    rt_device_control(_lcd_device, RTGRAPHIC_CTRL_GET_INFO, &info);
    lcd_frame_ptr = info.framebuffer;
    lcd_frame_write_index = 0;

    return 0;
}

int _lcd_frame_draw(void) {
    struct rt_device_rect_info rect_info;
    rect_info.x = 0;
    rect_info.y = 0;
    rect_info.width = 240;
    rect_info.height = 240;

    rt_device_control(_lcd_device, RTGRAPHIC_CTRL_RECT_UPDATE, &rect_info);
    return 0;
}

/**
 * @brief       ������Ϸ��ʾ����
 * @param       ��
 * @retval      ��
 */
void nes_set_window(void)
{
    uint16_t xoff = 0,yoff = 0;
    uint16_t lcdwidth,lcdheight;

    lcdwidth = 240;
    lcdheight = 240;
    nes_xoff = (256 - lcdwidth) / 2;    /* �õ�x�᷽���ƫ���� */

    // lcd_address_set(0, 0, lcdwidth - 1, lcdheight - 1);
}


typedef union {
    struct {
        uint16_t A1:1;
        uint16_t B1:1;
        uint16_t SE1:1;
        uint16_t ST1:1;
        uint16_t U1:1;
        uint16_t D1:1;
        uint16_t L1:1;
        uint16_t R1:1;

        uint16_t A2:1;
        uint16_t B2:1;
        uint16_t SE2:1;
        uint16_t ST2:1;
        uint16_t U2:1;
        uint16_t D2:1;
        uint16_t L2:1;
        uint16_t R2:1;
    };
    uint16_t joypad;
} nes_joypad_t;

/**
 * @brief       ��ȡ��Ϸ�ֱ�����
 * @param       ��
 * @retval      ��
 */
void nes_get_gamepadval(void)
{
    static nes_joypad_t joypad = {0};

    rt_uint32_t key_data = 0;
    if (rt_mb_recv(key_mb, &key_data, RT_WAITING_NO) == RT_EOK) {
        switch (key_data & 0xff00) {
            case 0x0100:
                switch (key_data & 0xff){
                    case 26://W
                        joypad.U1 = 1;
                        break;
                    case 22://S
                        joypad.D1 = 1;
                        break;
                    case 4://A
                        joypad.L1 = 1;
                        break;
                    case 7://D
                        joypad.R1 = 1;
                        break;
                    case 13://J
                        joypad.A1 = 1;
                        break;
                    case 14://K
                        joypad.B1 = 1;
                        break;
                    case 25://V
                        joypad.SE1 = 1;
                        break;
                    case 5://B
                        joypad.ST1 = 1;
                        break;
                    case 82://��
                        joypad.U2 = 1;
                        break;
                    case 81://��
                        joypad.D2 = 1;
                        break;
                    case 80://��
                        joypad.L2 = 1;
                        break;
                    case 79://��
                        joypad.R2 = 1;
                        break;
                    case 93://5
                        joypad.A2 = 1;
                        break;
                    case 94://6
                        joypad.B2 = 1;
                        break;
                    case 89://1
                        joypad.SE2 = 1;
                        break;
                    case 90://2
                        joypad.ST2 = 1;
                        break;
                    default:
                        break;
                    }
                break;
            case 0x0000:
                switch (key_data & 0xff){
                    case 26://W
                        joypad.U1 = 0;
                        break;
                    case 22://S
                        joypad.D1 = 0;
                        break;
                    case 4://A
                        joypad.L1 = 0;
                        break;
                    case 7://D
                        joypad.R1 = 0;
                        break;
                    case 13://J
                        joypad.A1 = 0;
                        break;
                    case 14://K
                        joypad.B1 = 0;
                        break;
                    case 25://V
                        joypad.SE1 = 0;
                        break;
                    case 5://B
                        joypad.ST1 = 0;
                        break;
                    case 82://��
                        joypad.U2 = 0;
                        break;
                    case 81://��
                        joypad.D2 = 0;
                        break;
                    case 80://��
                        joypad.L2 = 0;
                        break;
                    case 79://��
                        joypad.R2 = 0;
                        break;
                    case 93://5
                        joypad.A2 = 0;
                        break;
                    case 94://6
                        joypad.B2 = 0;
                        break;
                    case 89://1
                        joypad.SE2 = 0;
                        break;
                    case 90://2
                        joypad.ST2 = 0;
                        break;

                    case 41://ESC
                        system_task_return = 1;
                        break;
                    default:
                        break;
                    }
                break;

            default:
                break;
        }
    }

    PADdata0 = joypad.joypad; /* �ֱ�1 */
    PADdata1 = joypad.joypad >> 8; /* �ֱ�2 */
}

/**
 * @brief       nesģ������ѭ��
 * @param       ��
 * @retval      ��
 */
void nes_emulate_frame(void)
{
    uint8_t nes_frame;
    nes_set_window();               /* ���ô��� */

    while (1)
    {
        rt_thread_mdelay(2);

        /*  LINES 0-239 */
        PPU_start_frame();

        if (nes_frame == 0) {
            _lcd_get_framebuffer(); /* ��ȡLCD��ʾ���� */
        }

        rt_tick_t t1 = rt_tick_get();
        for (NES_scanline = 0; NES_scanline < 240; NES_scanline++)
        {
            run6502(113*256);
            NES_Mapper->HSync(NES_scanline);
            /* ɨ��һ�� */
            if (nes_frame == 0) scanline_draw(NES_scanline);
            else do_scanline_and_dont_draw(NES_scanline);
        }
        rt_tick_t t2 = rt_tick_get();
        rt_tick_t t_diff = t2 - t1;
        rt_kprintf("nes frame time:%u\r\n", t_diff);

        if (nes_frame == 0) {
            _lcd_frame_draw(); /* ˢ��LCD��ʾ���� */
        }

        NES_scanline = 240;
        run6502(113 * 256);/* ����1�� */
        NES_Mapper->HSync(NES_scanline);
        start_vblank();

        if (NMI_enabled())
        {
            cpunmi=1;
            run6502(7*256);/* �����ж� */
        }

        NES_Mapper->VSync();
        /*  LINES 242-261 */
        for (NES_scanline = 241;NES_scanline < 262;NES_scanline ++)
        {
            run6502(113*256);
            NES_Mapper->HSync(NES_scanline);
        }

        end_vblank();
        nes_get_gamepadval();   /* ÿ3֡��ѯһ��USB */
        apu_soundoutput();  /* �����Ϸ���� */
        g_framecnt++;
        nes_frame++;

        if (nes_frame > NES_SKIP_FRAME) nes_frame = 0;/* ��֡ */

        if (system_task_return)
        {
            printf("nes exit\r\n");
        }

        nes_set_window();               /* ���ô��� */
    }
}

/**
 * @brief       6502�������
 *  @note       ��6502.s���汻����
 * @param       reg0            : �Ĵ���0
 * @param       reg1            : �Ĵ���1
 * @retval      ��
 */
void debug_6502(uint16_t reg0,uint8_t reg1)
{
    printf("6502 error:%x,%d\r\n",reg0,reg1);
}


/**
 * @brief       NES����Ƶ���
 * @param       samples_per_sync: δ�õ�
 * @param       sample_rate     : ��Ƶ������
 * @retval      ��
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
 * @brief       NES�ر���Ƶ���
 * @param       ��
 * @retval      ��
 */
void nes_sound_close(void)
{
    if (_sound_device != RT_NULL) {
        rt_device_close(_sound_device);
        _sound_device = RT_NULL;
    }
}

/**
 * @brief       NES��Ƶ�����SAI����
 * @param       ��
 * @retval      ��
 */
void nes_apu_fill_buffer(int samples,uint16_t* wavebuf)
{
    rt_device_write(_sound_device, 0, wavebuf, APU_PCMBUF_SIZE * 2); /* �������ݵ���Ƶ�豸 */
}
