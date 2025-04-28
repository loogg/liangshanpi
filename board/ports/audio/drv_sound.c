#include "drv_sound.h"
#include <drv_common.h>

#define DBG_TAG "drv.sound"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

#define TX_DMA_FIFO_SIZE (2048)

struct stm32_audio {
    struct rt_audio_device audio;
    struct rt_audio_configure replay_config;
    int replay_volume;
    rt_uint8_t *tx_fifo;
    rt_bool_t startup;
};
struct stm32_audio _stm32_audio_play = {0};

TIM_HandleTypeDef htim6;
DAC_HandleTypeDef hdac;
DMA_HandleTypeDef hdma_dac2;

/* sample_rate, arr, psc */
static const rt_uint32_t _PSC_TBL[][3] = {
#if defined(BSP_CPU_CLK_168MHZ)
    {AUDIO_FREQUENCY_048K, 10 - 1, 175 - 1},
    {AUDIO_FREQUENCY_044K, 15 - 1, 127 - 1},
    {AUDIO_FREQUENCY_032K, 15 - 1, 175 - 1},
    {AUDIO_FREQUENCY_022K, 10 - 1, 381 - 1},
    {AUDIO_FREQUENCY_016K, 10 - 1, 525 - 1},
    {AUDIO_FREQUENCY_011K, 20 - 1, 381 - 1},
    {AUDIO_FREQUENCY_008K, 100 - 1, 105 - 1},
#elif defined(BSP_CPU_CLK_240MHZ)
    {AUDIO_FREQUENCY_048K, 10 - 1, 250 - 1},
    {AUDIO_FREQUENCY_044K, 3 - 1,  907 - 1},
    {AUDIO_FREQUENCY_032K, 10 - 1, 375 - 1},
    {AUDIO_FREQUENCY_022K, 6 - 1,  907 - 1},
    {AUDIO_FREQUENCY_016K, 10 - 1, 750 - 1},
    {AUDIO_FREQUENCY_011K, 12 - 1, 907 - 1},
    {AUDIO_FREQUENCY_008K, 100 - 1, 150 - 1},
#endif
};

static void MX_TIM6_Init(void) {
    /* USER CODE BEGIN TIM6_Init 0 */

    /* USER CODE END TIM6_Init 0 */

    TIM_MasterConfigTypeDef sMasterConfig = {0};

    /* USER CODE BEGIN TIM6_Init 1 */

    /* USER CODE END TIM6_Init 1 */
    htim6.Instance = TIM6;
    htim6.Init.Prescaler = 0;
    htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim6.Init.Period = 65535;
    htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim6) != HAL_OK) {
        Error_Handler();
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK) {
        Error_Handler();
    }
    /* USER CODE BEGIN TIM6_Init 2 */
    __HAL_TIM_CLEAR_FLAG(&htim6, TIM_FLAG_UPDATE); /* clear update flag */
    __HAL_TIM_URS_ENABLE(&htim6);                  /* enable update request source */
                                                   /* USER CODE END TIM6_Init 2 */
}

static void MX_DMA_Init(void) {
    /* DMA controller clock enable */
    __HAL_RCC_DMA1_CLK_ENABLE();

    /* DAC DMA Init */
    /* DAC2 Init */
    hdma_dac2.Instance = DMA1_Stream6;
    hdma_dac2.Init.Channel = DMA_CHANNEL_7;
    hdma_dac2.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_dac2.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_dac2.Init.MemInc = DMA_MINC_ENABLE;
    hdma_dac2.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_dac2.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_dac2.Init.Mode = DMA_CIRCULAR;
    hdma_dac2.Init.Priority = DMA_PRIORITY_HIGH;
    hdma_dac2.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_dac2) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(&hdac,DMA_Handle2,hdma_dac2);

    /* DMA interrupt init */
    /* DMA1_Stream6_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(DMA1_Stream6_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream6_IRQn);
}

static void MX_DAC_Init(void) {
    /* USER CODE BEGIN DAC_Init 0 */

    /* USER CODE END DAC_Init 0 */

    DAC_ChannelConfTypeDef sConfig = {0};

    /* USER CODE BEGIN DAC_Init 1 */

    /* USER CODE END DAC_Init 1 */

    /** DAC Initialization
     */
    hdac.Instance = DAC;
    if (HAL_DAC_Init(&hdac) != HAL_OK) {
        Error_Handler();
    }

    /** DAC channel OUT2 config
     */
    sConfig.DAC_Trigger = DAC_TRIGGER_T6_TRGO;
    sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_DISABLE;
    if (HAL_DAC_ConfigChannel(&hdac, &sConfig, DAC_CHANNEL_2) != HAL_OK) {
        Error_Handler();
    }
    /* USER CODE BEGIN DAC_Init 2 */

    /* USER CODE END DAC_Init 2 */
}

static void _samplerate_set(rt_uint32_t freq) {
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct;
    int i;

    /* check frequence */
    for (i = 0; i < (sizeof(_PSC_TBL) / sizeof(_PSC_TBL[0])); i++) {
        if ((freq) == _PSC_TBL[i][0]) break;
    }
    if (i == (sizeof(_PSC_TBL) / sizeof(_PSC_TBL[0]))) {
        LOG_E("Can not support this frequence: %d.", freq);
        return;
    }

    HAL_TIM_Base_Stop(&htim6);
    __HAL_TIM_SET_COUNTER(&htim6, 0);
    __HAL_TIM_SET_AUTORELOAD(&htim6, _PSC_TBL[i][1]);
    __HAL_TIM_SET_PRESCALER(&htim6, _PSC_TBL[i][2]);
    htim6.Instance->EGR |= TIM_EVENTSOURCE_UPDATE;
    HAL_TIM_Base_Start(&htim6);
}

static void _dac_dma_update(void) {
    if (_stm32_audio_play.replay_config.channels == 1) {
        switch (_stm32_audio_play.replay_config.samplebits) {
            case 8: {
                hdma_dac2.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
                hdma_dac2.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
            } break;

            case 16: {
                hdma_dac2.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
                hdma_dac2.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
            } break;

            default:
                LOG_E("Can not support this samplebits: %d.", _stm32_audio_play.replay_config.samplebits);
                return;
        }
    } else {
        switch (_stm32_audio_play.replay_config.samplebits) {
            case 8: {
                hdma_dac2.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
                hdma_dac2.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
            } break;

            case 16: {
                hdma_dac2.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
                hdma_dac2.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
            } break;

            default:
                LOG_E("Can not support this samplebits: %d.", _stm32_audio_play.replay_config.samplebits);
                return;
        }
    }

    HAL_DMA_Init(&hdma_dac2);
}

static void _channels_set(rt_uint16_t channels) {

    _dac_dma_update();
}

static void _samplebits_set(rt_uint16_t samplebits) {
    _dac_dma_update();
}

static void _config_set(struct rt_audio_configure config) {
    _channels_set(config.channels);
    _samplerate_set(config.samplerate);
    _samplebits_set(config.samplebits);
}

void DMA1_Stream6_IRQHandler(void) {
    rt_interrupt_enter();
    HAL_DMA_IRQHandler(&hdma_dac2);
    rt_interrupt_leave();
}

void HAL_DACEx_ConvHalfCpltCallbackCh2(DAC_HandleTypeDef *hdac) {
    rt_audio_tx_complete(&_stm32_audio_play.audio);
}

void HAL_DACEx_ConvCpltCallbackCh2(DAC_HandleTypeDef *hdac) {
    rt_audio_tx_complete(&_stm32_audio_play.audio);
}

static rt_err_t stm32_player_getcaps(struct rt_audio_device *audio, struct rt_audio_caps *caps) {
    rt_err_t result = RT_EOK;
    struct stm32_audio *st_audio = (struct stm32_audio *)audio->parent.user_data;

    LOG_D("%s:main_type: %d, sub_type: %d", __FUNCTION__, caps->main_type, caps->sub_type);

    switch (caps->main_type) {
        case AUDIO_TYPE_QUERY: /* query the types of hw_codec device */
        {
            switch (caps->sub_type) {
                case AUDIO_TYPE_QUERY:
                    caps->udata.mask = AUDIO_TYPE_OUTPUT | AUDIO_TYPE_MIXER;
                    break;

                default:
                    result = -RT_ERROR;
                    break;
            }

            break;
        }

        case AUDIO_TYPE_OUTPUT: /* Provide capabilities of OUTPUT unit */
        {
            switch (caps->sub_type) {
                case AUDIO_DSP_PARAM:
                    caps->udata.config.channels = st_audio->replay_config.channels;
                    caps->udata.config.samplebits = st_audio->replay_config.samplebits;
                    caps->udata.config.samplerate = st_audio->replay_config.samplerate;
                    break;

                case AUDIO_DSP_SAMPLERATE:
                    caps->udata.config.samplerate = st_audio->replay_config.samplerate;
                    break;

                case AUDIO_DSP_CHANNELS:
                    caps->udata.config.channels = st_audio->replay_config.channels;
                    break;

                case AUDIO_DSP_SAMPLEBITS:
                    caps->udata.config.samplebits = st_audio->replay_config.samplebits;
                    break;

                default:
                    result = -RT_ERROR;
                    break;
            }

            break;
        }

        case AUDIO_TYPE_MIXER: /* report the Mixer Units */
        {
            switch (caps->sub_type) {
                case AUDIO_MIXER_QUERY:
                    caps->udata.mask = AUDIO_MIXER_VOLUME | AUDIO_MIXER_LINE;
                    break;

                case AUDIO_MIXER_VOLUME:
                    caps->udata.value = st_audio->replay_volume;
                    break;

                case AUDIO_MIXER_LINE:
                    break;

                default:
                    result = -RT_ERROR;
                    break;
            }

            break;
        }

        default:
            result = -RT_ERROR;
            break;
    }

    return result;
}

static rt_err_t stm32_player_configure(struct rt_audio_device *audio, struct rt_audio_caps *caps) {
    rt_err_t result = RT_EOK;
    struct stm32_audio *st_audio = (struct stm32_audio *)audio->parent.user_data;

    LOG_D("%s:main_type: %d, sub_type: %d", __FUNCTION__, caps->main_type, caps->sub_type);

    switch (caps->main_type) {
        case AUDIO_TYPE_MIXER: {
            switch (caps->sub_type) {
                case AUDIO_MIXER_MUTE: {
                    /* set mute mode */

                    break;
                }

                case AUDIO_MIXER_VOLUME: {
                    int volume = caps->udata.value;

                    st_audio->replay_volume = volume;
                    /* set mixer volume */

                    break;
                }

                default:
                    result = -RT_ERROR;
                    break;
            }

            break;
        }

        case AUDIO_TYPE_OUTPUT: {
            switch (caps->sub_type) {
                case AUDIO_DSP_PARAM: {
                    struct rt_audio_configure config = caps->udata.config;

                    st_audio->replay_config.samplerate = config.samplerate;
                    st_audio->replay_config.samplebits = config.samplebits;
                    st_audio->replay_config.channels = config.channels;

                    _config_set(config);
                    break;
                }

                case AUDIO_DSP_SAMPLERATE: {
                    st_audio->replay_config.samplerate = caps->udata.config.samplerate;
                    _samplerate_set(caps->udata.config.samplerate);
                    break;
                }

                case AUDIO_DSP_CHANNELS: {
                    st_audio->replay_config.channels = caps->udata.config.channels;
                    _channels_set(caps->udata.config.channels);
                    break;
                }

                case AUDIO_DSP_SAMPLEBITS: {
                    st_audio->replay_config.samplebits = caps->udata.config.samplebits;
                    _samplebits_set(caps->udata.config.samplebits);
                    break;
                }

                default:
                    result = -RT_ERROR;
                    break;
            }
            break;
        }

        default:
            break;
    }

    return result;
}

static rt_err_t stm32_player_init(struct rt_audio_device *audio) {
    MX_DMA_Init();
    MX_DAC_Init();
    MX_TIM6_Init();

    return RT_EOK;
}

static rt_err_t stm32_player_start(struct rt_audio_device *audio, int stream) {
    struct stm32_audio *st_audio = (struct stm32_audio *)audio->parent.user_data;

    if (stream == AUDIO_STREAM_REPLAY) {
        switch (st_audio->replay_config.samplebits) {
            case 8: {
                if (HAL_DAC_Start_DMA(&hdac, DAC_CHANNEL_2, (uint32_t *)_stm32_audio_play.tx_fifo,
                                      TX_DMA_FIFO_SIZE / st_audio->replay_config.channels, DAC_ALIGN_8B_R) != HAL_OK) {
                    /* Start DMA Error */
                    Error_Handler();
                }
            } break;

            case 16: {
                if (HAL_DAC_Start_DMA(&hdac, DAC_CHANNEL_2, (uint32_t *)_stm32_audio_play.tx_fifo,
                                      TX_DMA_FIFO_SIZE / 2 / st_audio->replay_config.channels, DAC_ALIGN_12B_L) != HAL_OK) {
                    /* Start DMA Error */
                    Error_Handler();
                }
            } break;

            default:
                break;
        }
    }

    return RT_EOK;
}

static rt_err_t stm32_player_stop(struct rt_audio_device *audio, int stream) {
    if (stream == AUDIO_STREAM_REPLAY) {
        HAL_DAC_Stop_DMA(&hdac, DAC_CHANNEL_2);
    }

    return RT_EOK;
}

static rt_err_t stm32_player_preprocess(struct rt_audio_device *audio, void *buffer, rt_size_t size) {
    struct stm32_audio *st_audio = (struct stm32_audio *)audio->parent.user_data;
    uint8_t *ptr = (uint8_t *)buffer;

    if (st_audio->replay_config.samplebits != 16)
        return RT_EOK;

    for (int i = 0; i + 1 < size; i += 2) {
        ptr[i + 1] = ptr[i + 1] - 0x80;
    }

    return RT_EOK;
}

static void stm32_player_buffer_info(struct rt_audio_device *audio, struct rt_audio_buf_info *info) {
    /**
     *               TX_FIFO
     * +----------------+----------------+
     * |     block1     |     block2     |
     * +----------------+----------------+
     *  \  block_size  /
     */
    info->buffer = _stm32_audio_play.tx_fifo;
    info->total_size = TX_DMA_FIFO_SIZE;
    info->block_size = TX_DMA_FIFO_SIZE / 2;
    info->block_count = 2;
}
static struct rt_audio_ops _p_audio_ops = {
    .getcaps = stm32_player_getcaps,
    .configure = stm32_player_configure,
    .init = stm32_player_init,
    .start = stm32_player_start,
    .stop = stm32_player_stop,
    .data_pre = stm32_player_preprocess,
    .transmit = RT_NULL,
    .buffer_info = stm32_player_buffer_info,
};

int rt_hw_sound_init(void) {
    rt_uint8_t *tx_fifo;

    /* player */
    tx_fifo = rt_malloc(TX_DMA_FIFO_SIZE);
    if (tx_fifo == RT_NULL) {
        return -RT_ENOMEM;
    }
    rt_memset(tx_fifo, 0, TX_DMA_FIFO_SIZE);
    _stm32_audio_play.tx_fifo = tx_fifo;

    /* register sound device */
    _stm32_audio_play.audio.ops = &_p_audio_ops;
    rt_audio_register(&_stm32_audio_play.audio, "sound0", RT_DEVICE_FLAG_WRONLY, &_stm32_audio_play);

    return RT_EOK;
}
INIT_DEVICE_EXPORT(rt_hw_sound_init);
