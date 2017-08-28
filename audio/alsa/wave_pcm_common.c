/*
 *  Copyright (C) 2017, Zhang YanMing <yanmin.zhang@ingenic.com, jamincheung@126.com>
 *
 *  Ingenic Linux plarform SDK project
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under  the terms of the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <utils/log.h>
#include <utils/common.h>
#include "wave_pcm_common.h"

#define LOG_TAG "wave_pcm_common"

uint32_t pcm_read(struct snd_pcm_container* pcm_container, uint32_t read_count) {
    uint32_t count = read_count;
    uint32_t readed;
    uint32_t read_sofar = 0;
    uint8_t *data = pcm_container->data_buf;

    if (count != pcm_container->chunk_size)
        count = pcm_container->chunk_size;

    while (count) {
        pthread_testcancel();

        readed = snd_pcm_readi(pcm_container->pcm_handle, data, count);
        if (readed == -EAGAIN || (readed >= 0 && readed < count)) {
            snd_pcm_wait(pcm_container->pcm_handle, 1000);

        } else if (readed == -EPIPE) {
            snd_pcm_prepare(pcm_container->pcm_handle);
            LOGW("Overrun occured\n");

        } else if (readed == -ESTRPIPE) {
            LOGW("Need suspend\n");

        } else if (readed < 0) {
            LOGE("Failed to snd_pcm_readi: %s\n", snd_strerror(readed));
            return -1;
        }

        if (readed > 0) {
            read_sofar += readed;
            count -= readed;
            data += readed * pcm_container->bits_per_frame / 8;
        }
    }

    return read_count;
}

uint32_t pcm_write(struct snd_pcm_container* pcm_container, uint32_t write_count) {
    uint32_t count = write_count;
    uint32_t writed;
    uint32_t write_sofar = 0;
    uint8_t* data = pcm_container->data_buf;


    if (count < pcm_container->chunk_size) {
        snd_pcm_format_set_silence(pcm_container->format,
                data + count * pcm_container->bits_per_frame / 8,
                (pcm_container->chunk_size - count) * pcm_container->channels);
        count = pcm_container->chunk_size;
    }

    while (count) {
        pthread_testcancel();

        writed = snd_pcm_writei(pcm_container->pcm_handle, data, count);
        if (writed == -EAGAIN || (writed >= 0 && writed < count)) {
            snd_pcm_wait(pcm_container->pcm_handle, 1000);

        } else if (writed == -EPIPE) {
            snd_pcm_prepare(pcm_container->pcm_handle);
            LOGW("Underrun occured\n");

        } else if (writed == -ESTRPIPE) {
            LOGW("Need suspend\n");

        } else if (writed < 0) {
            LOGE("Failed to snd_pcm_writei: %s\n", snd_strerror(writed));
            return -1;
        }

        if (writed > 0) {
            write_sofar += writed;
            count -= writed;
            data += writed * pcm_container->bits_per_frame / 8;
        }
    }

    return write_sofar;
}

int pcm_set_params(struct snd_pcm_container* pcm_container,
        uint16_t bit_per_spl, uint16_t channles, uint32_t sample_fq) {
    int error = 0;
    snd_pcm_hw_params_t* hw_params;
    snd_pcm_sw_params_t *sw_params;
    snd_pcm_format_t format;
    uint32_t exact_rate;
    uint32_t buffer_time;
    uint32_t period_time;

    snd_pcm_hw_params_alloca(&hw_params);
    snd_pcm_sw_params_alloca(&sw_params);

    error = snd_pcm_hw_params_any(pcm_container->pcm_handle, hw_params);
    if (error < 0) {
        LOGE("Failed to snd_pcm_hw_params_any: %s\n", snd_strerror(error));
        return -1;
    }

    error = snd_pcm_hw_params_set_rate_resample(pcm_container->pcm_handle,
            hw_params, 1);
    if (error < 0) {
        LOGE("Failed to setup resample: %s\n", snd_strerror(error));
        return -1;
    }

    error = snd_pcm_hw_params_set_access(pcm_container->pcm_handle, hw_params,
            SND_PCM_ACCESS_RW_INTERLEAVED);
    if (error < 0) {
        LOGE("Failed to snd_pcm_hw_params_set_access: %s\n", snd_strerror(error));
        return -1;
    }

    switch (bit_per_spl) {
    case 32:
        format = SND_PCM_FORMAT_S32_LE;
        break;
    case 24:
        format = SND_PCM_FORMAT_S24_LE;
        break;
    case 16:
        format = SND_PCM_FORMAT_S16_LE;
        break;
    case 8:
        format = SND_PCM_FORMAT_S8;
        break;
    default:
        format = SND_PCM_FORMAT_UNKNOWN;
        break;
    }

    error = snd_pcm_hw_params_set_format(pcm_container->pcm_handle, hw_params,
            format);
    if (error < 0) {
        LOGE("Failed to snd_pcm_hw_params_set_format: %s\n", snd_strerror(error));
        return -1;
    }

    pcm_container->format = format;

    error = snd_pcm_hw_params_set_channels(pcm_container->pcm_handle, hw_params,
            LE_SHORT(channles));
    if (error < 0) {
        LOGE("Failed to snd_pcm_hw_params_set_channels: %s\n", snd_strerror(error));
        return -1;
    }

    pcm_container->channels = LE_SHORT(channles);

    exact_rate = LE_INT(sample_fq);
    error = snd_pcm_hw_params_set_rate_near(pcm_container->pcm_handle, hw_params,
            &exact_rate, 0);
    if (error < 0) {
        LOGE("Failed to snd_pcm_hw_params_set_rate_near: %s\n", snd_strerror(error));
        return -1;
    }
    if (LE_INT(sample_fq) != exact_rate) {
        LOGW("Sample rate %dHz is not supported, using %dHz instead.\n",
            LE_INT(sample_fq), exact_rate);
    }

    error = snd_pcm_hw_params_get_buffer_time_max(hw_params, &buffer_time, 0);
    if (error < 0) {
        LOGE("Failed to snd_pcm_hw_params_get_buffer_time_max: %s\n", snd_strerror(error));
        return -1;
    }

    if (buffer_time > 500000)
        buffer_time = 500000;
    period_time = buffer_time / 4;

    error = snd_pcm_hw_params_set_buffer_time_near(pcm_container->pcm_handle,
            hw_params, &buffer_time, 0);
    if (error < 0) {
        LOGE("Failed to snd_pcm_hw_params_set_buffer_time_near: %s\n", snd_strerror(error));
        return -1;
    }

    error = snd_pcm_hw_params_set_period_time_near(pcm_container->pcm_handle,
            hw_params, &period_time, 0);
    if (error < 0) {
        LOGE("Failed to snd_pcm_hw_params_set_period_time_near: %s\n", snd_strerror(error));
        return -1;
    }

    error = snd_pcm_hw_params(pcm_container->pcm_handle, hw_params);
    if (error < 0) {
        LOGE("Failed to snd_pcm_hw_params: %s\n", snd_strerror(error));
        return -1;
    }

    snd_pcm_hw_params_get_period_size(hw_params, &pcm_container->chunk_size, 0);
    snd_pcm_hw_params_get_buffer_size(hw_params, &pcm_container->buffer_size);
    if (pcm_container->chunk_size == pcm_container->buffer_size) {
        LOGE("Can't use period equal to buffer size (%lu == %lu)\n",
                pcm_container->chunk_size, pcm_container->buffer_size);
        return -1;
    }

    snd_pcm_sw_params_current(pcm_container->pcm_handle, sw_params);

    error = snd_pcm_sw_params_set_start_threshold(pcm_container->pcm_handle,
            sw_params, 0);
    if (error < 0) {
        LOGE("Failed to set start threshold\n");
        return -1;
    }

    error = snd_pcm_sw_params_set_stop_threshold(pcm_container->pcm_handle,
            sw_params, pcm_container->buffer_size);
    if (error < 0) {
        LOGE("Failed to set stop threshold\n");
        return -1;
    }

    error = snd_pcm_sw_params(pcm_container->pcm_handle, sw_params);
    if (error < 0) {
        LOGE("Failed to snd_pcm_sw_params: %s\n", snd_strerror(error));
        return -1;
    }

    pcm_container->bits_per_sample = snd_pcm_format_physical_width(format);
    pcm_container->bits_per_frame = pcm_container->bits_per_sample
            * LE_SHORT(channles);

    pcm_container->chunk_bytes = pcm_container->chunk_size
            * pcm_container->bits_per_frame / 8;

    if (pcm_container->data_buf == NULL) {
        pcm_container->data_buf = (uint8_t *)malloc(pcm_container->chunk_bytes);
        if (!pcm_container->data_buf) {
            LOGE("Failed to malloc data_buffer\n");
            return -1;
        }
    }

    return 0;
}

int pcm_pause(struct snd_pcm_container* pcm_container) {
    int error = 0;
    snd_pcm_hw_params_t* hw_params;

    snd_pcm_hw_params_alloca(&hw_params);

    snd_pcm_hw_params_current(pcm_container->pcm_handle, hw_params);

    if (snd_pcm_hw_params_can_pause(hw_params))
        error = snd_pcm_pause(pcm_container->pcm_handle, 1);
    else
        error = snd_pcm_drop(pcm_container->pcm_handle);

    return error;
}

int pcm_resume(struct snd_pcm_container* pcm_container) {
    int error = 0;
    snd_pcm_hw_params_t* hw_params;

    snd_pcm_hw_params_alloca(&hw_params);

    snd_pcm_hw_params_current(pcm_container->pcm_handle, hw_params);

    if (snd_pcm_state(pcm_container->pcm_handle) == SND_PCM_STATE_SUSPENDED) {

        while ((error = snd_pcm_resume(pcm_container->pcm_handle)) == -EAGAIN)
            msleep(50);

    } else {
        if (snd_pcm_hw_params_can_pause(hw_params))
            error = snd_pcm_pause(pcm_container->pcm_handle, 0);
        else
            error = snd_pcm_prepare(pcm_container->pcm_handle);
    }

    return error;
}

int pcm_cancel(struct snd_pcm_container* pcm_container) {
    snd_pcm_drop(pcm_container->pcm_handle);
    return 0;
}
