/*
 * Copyright 2023-2024 Dozingfiretruck
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "nesplayer.h"
#include <drv_lcd.h>
#include <rtdevice.h>
#include "lv_port.h"

#define DBG_TAG    "nes.port"
#define DBG_LVL    DBG_INFO
#include <rtdbg.h>

#define SOUND_DEVICE_NAME   "sound0"

static rt_device_t _sound_device = RT_NULL;

enum {
    NESPLAYER_MSG_NONE = 0,
    NESPLAYER_MSG_START,
    NESPLAYER_MSG_STOP,
};

enum {
    NESPLAYER_STATE_STOPPED = 0,
    NESPLAYER_STATE_PLAYING = 1,
};

typedef struct _nesplayer {
    rt_mailbox_t key_mb;
    char *uri;
    rt_mailbox_t play_mb;
    uint8_t state;
} nesplayer_t;

static nesplayer_t _player = {0};
static uint16_t *_lcd_frame_buf1 = RT_NULL;
#ifdef BSP_NES_FRESH_USING_DOUBLE_BUFFER
static uint16_t *_lcd_frame_buf2 = RT_NULL;
#endif
static uint16_t *_lcd_framebuffer = RT_NULL;

#ifdef BSP_NES_FRESH_USING_LVGL_IMG
extern void lv_game_img_draw(void *pcolor);
extern void lv_game_img_exit(void);
#endif

#ifdef BSP_USING_NES_C
/* memory */
void *nes_malloc(int num){
    return rt_malloc(num);
}

void nes_free(void *address){
    rt_free(address);
}

void *nes_memcpy(void *str1, const void *str2, size_t n){
    return memcpy(str1, str2, n);
}

void *nes_memset(void *str, int c, size_t n){
    return memset(str,c,n);
}

int nes_memcmp(const void *str1, const void *str2, size_t n){
    return memcmp(str1,str2,n);
}

#if (NES_USE_FS == 1)
/* io */
FILE *nes_fopen(const char * filename, const char * mode ){
    return fopen(filename,mode);
}

size_t nes_fread(void *ptr, size_t size, size_t nmemb, FILE *stream){
    return fread(ptr, size, nmemb,stream);
}

size_t nes_fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream){
    return fwrite(ptr, size, nmemb,stream);
}

int nes_fseek(FILE *stream, long int offset, int whence){
    return fseek(stream,offset,whence);
}

int nes_fclose(FILE *stream ){
    return fclose(stream);
}
#endif

static void update_joypad(nes_t *nes)
{
    rt_uint32_t key_data = 0;
    if (rt_mb_recv(_player.key_mb, &key_data, RT_WAITING_NO) == RT_EOK) {
        switch (key_data & 0xff00) {
            case 0x0100:
                switch (key_data & 0xff){
                    case 26://W
                        nes->nes_cpu.joypad.U1 = 1;
                        break;
                    case 22://S
                        nes->nes_cpu.joypad.D1 = 1;
                        break;
                    case 4://A
                        nes->nes_cpu.joypad.L1 = 1;
                        break;
                    case 7://D
                        nes->nes_cpu.joypad.R1 = 1;
                        break;
                    case 13://J
                        nes->nes_cpu.joypad.A1 = 1;
                        break;
                    case 14://K
                        nes->nes_cpu.joypad.B1 = 1;
                        break;
                    case 25://V
                        nes->nes_cpu.joypad.SE1 = 1;
                        break;
                    case 5://B
                        nes->nes_cpu.joypad.ST1 = 1;
                        break;
                    case 82://↑
                        nes->nes_cpu.joypad.U2 = 1;
                        break;
                    case 81://↓
                        nes->nes_cpu.joypad.D2 = 1;
                        break;
                    case 80://←
                        nes->nes_cpu.joypad.L2 = 1;
                        break;
                    case 79://→
                        nes->nes_cpu.joypad.R2 = 1;
                        break;
                    case 93://5
                        nes->nes_cpu.joypad.A2 = 1;
                        break;
                    case 94://6
                        nes->nes_cpu.joypad.B2 = 1;
                        break;
                    case 89://1
                        nes->nes_cpu.joypad.SE2 = 1;
                        break;
                    case 90://2
                        nes->nes_cpu.joypad.ST2 = 1;
                        break;

                    default:
                        break;
                    }
                break;
            case 0x0000:
                switch (key_data & 0xff){
                    case 26://W
                        nes->nes_cpu.joypad.U1 = 0;
                        break;
                    case 22://S
                        nes->nes_cpu.joypad.D1 = 0;
                        break;
                    case 4://A
                        nes->nes_cpu.joypad.L1 = 0;
                        break;
                    case 7://D
                        nes->nes_cpu.joypad.R1 = 0;
                        break;
                    case 13://J
                        nes->nes_cpu.joypad.A1 = 0;
                        break;
                    case 14://K
                        nes->nes_cpu.joypad.B1 = 0;
                        break;
                    case 25://V
                        nes->nes_cpu.joypad.SE1 = 0;
                        break;
                    case 5://B
                        nes->nes_cpu.joypad.ST1 = 0;
                        break;
                    case 82://↑
                        nes->nes_cpu.joypad.U2 = 0;
                        break;
                    case 81://↓
                        nes->nes_cpu.joypad.D2 = 0;
                        break;
                    case 80://←
                        nes->nes_cpu.joypad.L2 = 0;
                        break;
                    case 79://→
                        nes->nes_cpu.joypad.R2 = 0;
                        break;
                    case 93://5
                        nes->nes_cpu.joypad.A2 = 0;
                        break;
                    case 94://6
                        nes->nes_cpu.joypad.B2 = 0;
                        break;
                    case 89://1
                        nes->nes_cpu.joypad.SE2 = 0;
                        break;
                    case 90://2
                        nes->nes_cpu.joypad.ST2 = 0;
                        break;

                    case 41://ESC
                        nes->nes_quit = 1;
                        break;

                    default:
                        break;
                    }
                break;

            default:
                break;
        }
    }
}

#if (NES_ENABLE_SOUND == 1)

int nes_sound_output(uint8_t *buffer, size_t len){
    rt_device_write(_sound_device, 0, buffer, len);

    return 0;
}
#endif

int nes_initex(nes_t *nes){
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
    caps.udata.config.samplerate = NES_APU_SAMPLE_RATE;
    caps.udata.config.channels = 1;
    caps.udata.config.samplebits = 8;
    rt_device_control(_sound_device, AUDIO_CTL_CONFIGURE, &caps);

    return 0;
}

int nes_deinitex(nes_t *nes){
    if (_sound_device != RT_NULL) {
        rt_device_close(_sound_device);
        _sound_device = RT_NULL;
    }

    return 0;
}

int nes_get_framebuffer(nes_t *nes) {
#ifdef BSP_NES_FRESH_USING_DOUBLE_BUFFER
    if (_lcd_framebuffer == _lcd_frame_buf1) {
        _lcd_framebuffer = _lcd_frame_buf2;
    } else {
        _lcd_framebuffer = _lcd_frame_buf1;
    }
#endif

#ifdef BSP_NES_FRESH_USING_LVGL_IMG
    _lcd_framebuffer = _lcd_frame_buf1;
#endif

    nes->nes_draw_data = _lcd_framebuffer;

    return 0;
}

int nes_draw(int x1, int y1, int x2, int y2, nes_color_t* color_data){
#ifdef BSP_NES_FRESH_USING_DOUBLE_BUFFER
    lcd_fill_array_async(x1, y1 + 10, x2, y2 + 10, color_data);
#endif

#ifdef BSP_NES_FRESH_USING_LVGL_IMG
    lv_game_img_draw(color_data);
#endif

    return 0;
}

// #define FRAMES_PER_SECOND   1000/60

void nes_frame(nes_t* nes){
    rt_uint32_t mb_data = 0;
    if (rt_mb_recv(_player.play_mb, &mb_data, RT_WAITING_NO) == RT_EOK) {
        switch (mb_data) {
            case NESPLAYER_MSG_STOP: {
                nes->nes_quit = 1;
            } break;

            default:
                break;
        }
    }

    update_joypad(nes);
    rt_thread_mdelay(1);
}
#endif /* BSP_USING_NES_C */

#ifdef BSP_USING_NES_OPENEDV

extern uint16_t *lcd_frame_ptr;
extern uint32_t lcd_frame_write_index;

int nes_get_framebuffer(void) {
#ifdef BSP_NES_FRESH_USING_DOUBLE_BUFFER
    if (_lcd_framebuffer == _lcd_frame_buf1) {
        _lcd_framebuffer = _lcd_frame_buf2;
    } else {
        _lcd_framebuffer = _lcd_frame_buf1;
    }
#endif

#ifdef BSP_NES_FRESH_USING_LVGL_IMG
    _lcd_framebuffer = _lcd_frame_buf1;
#endif

    lcd_frame_ptr = _lcd_framebuffer;
    lcd_frame_write_index = 0;

    return 0;
}

int nes_frame_draw(void) {
#ifdef BSP_NES_FRESH_USING_DOUBLE_BUFFER
    uint16_t x1 = 0;
    uint16_t y1 = 0;
    uint16_t x2 = 240 - 1;
    uint16_t y2 = 240 - 1;

    lcd_fill_array_async(x1, y1 + 10, x2, y2 + 10, _lcd_framebuffer);
#endif

#ifdef BSP_NES_FRESH_USING_LVGL_IMG
    lv_game_img_draw(_lcd_framebuffer);
#endif

    return 0;
}

extern uint8_t PADdata0;
extern uint8_t PADdata1;
extern volatile uint8_t system_task_return;

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

static nes_joypad_t joypad = {0};

/**
 * @brief       读取游戏手柄数据
 * @param       无
 * @retval      无
 */
void nes_get_gamepadval(void)
{
    rt_uint32_t mb_data = 0;
    rt_uint32_t key_data = 0;

    if (rt_mb_recv(_player.play_mb, &mb_data, RT_WAITING_NO) == RT_EOK) {
        switch (mb_data) {
            case NESPLAYER_MSG_STOP: {
                system_task_return = 1;
            } break;

            default:
                break;
        }
    }

    if (rt_mb_recv(_player.key_mb, &key_data, RT_WAITING_NO) == RT_EOK) {
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
                    case 82://↑
                        joypad.U2 = 1;
                        break;
                    case 81://↓
                        joypad.D2 = 1;
                        break;
                    case 80://←
                        joypad.L2 = 1;
                        break;
                    case 79://→
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
                    case 82://↑
                        joypad.U2 = 0;
                        break;
                    case 81://↓
                        joypad.D2 = 0;
                        break;
                    case 80://←
                        joypad.L2 = 0;
                        break;
                    case 79://→
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

    PADdata0 = joypad.joypad; /* 手柄1 */
    PADdata1 = joypad.joypad >> 8; /* 手柄2 */
}

#endif /* BSP_USING_NES_OPENEDV */

int nesplayer_play(char *uri) {
    if (_player.state != NESPLAYER_STATE_STOPPED) {
        nesplayer_stop();
    }

    if (_player.uri)
    {
        rt_free(_player.uri);
    }
    _player.uri = rt_strdup(uri);
    rt_mb_send(_player.play_mb, NESPLAYER_MSG_START);

    return 0;
}

int nesplayer_stop(void) {
    if (_player.state != NESPLAYER_STATE_STOPPED) {
        rt_mb_send(_player.play_mb, NESPLAYER_MSG_STOP);
    }

    return 0;
}

int nesplayer_send_key_event(uint32_t key_event) {
    if (_player.state == NESPLAYER_STATE_PLAYING) {
        rt_mb_send(_player.key_mb, key_event);
    }

    return RT_EOK;
}

static void nesplayer_entry(void* parameter) {
    rt_uint32_t mb_data = 0;

    while (1) {
        if (rt_mb_recv(_player.play_mb, &mb_data, RT_WAITING_FOREVER) != RT_EOK) continue;
        if (mb_data != NESPLAYER_MSG_START) continue;

        rt_mb_control(_player.key_mb, RT_IPC_CMD_RESET, RT_NULL);
        _player.state = NESPLAYER_STATE_PLAYING;

#ifdef BSP_USING_NES_C
        nes_t *nes = nes_init();
        do {
            int ret = nes_load_file(nes, _player.uri);
            if (ret) {
                LOG_E("nes load file fail");
                break;
            }

            nes_run(nes);
            nes_unload_file(nes);
        } while (0);
        nes_deinit(nes);
#endif /* BSP_USING_NES_C */

#ifdef BSP_USING_NES_OPENEDV
        rt_memset(&joypad, 0, sizeof(nes_joypad_t));
        nes_load((uint8_t *)_player.uri);
#endif /* BSP_USING_NES_OPENEDV */

        _player.state = NESPLAYER_STATE_STOPPED;
#ifdef BSP_NES_FRESH_USING_DOUBLE_BUFFER
        disp_enable_update();
#endif

#ifdef BSP_NES_FRESH_USING_LVGL_IMG
        lv_game_img_exit();
#endif
    }
}

static struct rt_thread nesplayer_thread;

#ifdef rt_align
rt_align(RT_ALIGN_SIZE)
#else
ALIGN(RT_ALIGN_SIZE)
#endif
static rt_uint8_t nesplayer_thread_stack[4096];

static int nesplayer_init(void) {
    _lcd_frame_buf1 = rt_malloc(LCD_BUF_SIZE);
    if (_lcd_frame_buf1 == RT_NULL) {
        LOG_E("malloc lcd frame buffer failed");
        return -RT_ERROR;
    }
#ifdef BSP_NES_FRESH_USING_DOUBLE_BUFFER
    _lcd_frame_buf2 = rt_malloc(LCD_BUF_SIZE);
    if (_lcd_frame_buf2 == RT_NULL) {
        LOG_E("malloc lcd frame buffer failed");
        return -RT_ERROR;
    }
#endif
    _lcd_framebuffer = _lcd_frame_buf1;

    _player.key_mb = rt_mb_create("nes_kb", 32, RT_IPC_FLAG_FIFO);
    _player.play_mb = rt_mb_create("nes_pl", 32, RT_IPC_FLAG_FIFO);
    if (_player.key_mb == RT_NULL || _player.play_mb == RT_NULL) {
        LOG_E("create mailbox failed");
        return -RT_ERROR;
    }

    rt_thread_init(&nesplayer_thread, "nesplayer", nesplayer_entry, RT_NULL, &nesplayer_thread_stack[0], sizeof(nesplayer_thread_stack), 10, 10);
    rt_thread_startup(&nesplayer_thread);

    return RT_EOK;
}
INIT_APP_EXPORT(nesplayer_init);
