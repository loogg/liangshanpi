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

static uint16_t *_lcd_frame_buf1 = RT_NULL;
static uint16_t *_lcd_frame_buf2 = RT_NULL;
static uint16_t *_lcd_framebuffer = RT_NULL;

int nes_get_framebuffer(nes_t *nes) {
    if (_lcd_framebuffer == _lcd_frame_buf1) {
        _lcd_framebuffer = _lcd_frame_buf2;
    } else {
        _lcd_framebuffer = _lcd_frame_buf1;
    }

    nes->nes_draw_data = _lcd_framebuffer;

    return 0;
}

int nes_draw(int x1, int y1, int x2, int y2, nes_color_t* color_data){
    lcd_fill_array_async(x1, y1 + 10, x2, y2 + 10, _lcd_framebuffer);
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

        _player.state = NESPLAYER_STATE_STOPPED;
        disp_enable_update();
    }
}

static int nesplayer_init(void) {
    _lcd_frame_buf1 = rt_malloc(LCD_BUF_SIZE * 3);
    _lcd_frame_buf2 = rt_malloc(LCD_BUF_SIZE * 3);
    if (_lcd_frame_buf1 == RT_NULL || _lcd_frame_buf2 == RT_NULL) {
        LOG_E("malloc lcd frame buffer failed");
        return -RT_ERROR;
    }
    _lcd_framebuffer = _lcd_frame_buf1;

    _player.key_mb = rt_mb_create("nes_kb", 32, RT_IPC_FLAG_FIFO);
    _player.play_mb = rt_mb_create("nes_pl", 32, RT_IPC_FLAG_FIFO);
    if (_player.key_mb == RT_NULL || _player.play_mb == RT_NULL) {
        LOG_E("create mailbox failed");
        return -RT_ERROR;
    }

    rt_thread_t tid = rt_thread_create("nes_p", nesplayer_entry, RT_NULL, 4096, 10, 10);
    if (tid == RT_NULL) {
        LOG_E("create nes player thread failed");
        return -RT_ERROR;
    }
    rt_thread_startup(tid);

    return RT_EOK;
}
INIT_COMPONENT_EXPORT(nesplayer_init);
