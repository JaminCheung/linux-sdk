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
#include <utils/assert.h>
#include <utils/common.h>
#include <audio/alsa/wave_recorder.h>
#include "wave_pcm_common.h"

#define LOG_TAG "wave_recoder"

static WaveContainer wave_container;
static struct snd_pcm_container pcm_container;
static int hw_inited;
static uint32_t init_count;

static pthread_mutex_t init_lock = PTHREAD_MUTEX_INITIALIZER;

static int prepare_wave_params(WaveContainer *wav, int channels, int sample_rate,
        int sample_length, int duration_time) {

    wav->header.magic = WAV_RIFF;
    wav->header.type = WAV_WAVE;
    wav->format.magic = WAV_FMT;
    wav->format.format_size = LE_INT(16);
    wav->format.format = LE_SHORT(WAV_FMT_PCM);
    wav->chunk_header.type = WAV_DATA;

    wav->format.channels = LE_SHORT(channels);
    wav->format.sample_fq = LE_INT(sample_rate);
    wav->format.bit_p_spl = LE_SHORT(sample_length);

    wav->format.byte_p_spl = LE_SHORT(channels * sample_length / 8);
    wav->format.byte_p_sec = LE_INT((uint16_t)(wav->format.byte_p_spl) * sample_rate);

    wav->chunk_header.length = LE_INT(duration_time * (uint32_t)(wav->format.byte_p_sec));
    wav->header.length = LE_INT((uint32_t)(wav->chunk_header.length) +
        sizeof(wav->chunk_header) + sizeof(wav->format) + sizeof(wav->header) - 8);

    return 0;
}

static int do_record_wave(struct snd_pcm_container* pcm_container,
        WaveContainer* wave_container, int fd) {
    int error = 0;
    uint64_t count;
    uint32_t writed;
    uint32_t frame_count;

    error = wave_write_header(fd, wave_container);
    if (error < 0) {
        LOGE("Failed to write wave header\n");
        return -1;
    }

    count = wave_container->chunk_header.length;
    while (count) {
        writed = (count <= pcm_container->chunk_bytes) ? count : pcm_container->chunk_bytes;
        frame_count = writed * 8 / pcm_container->bits_per_frame;

        if (pcm_read(pcm_container, frame_count) != frame_count)
            break;

        if (write(fd, pcm_container->data_buf, writed) != writed) {
            LOGE("Failed to write wave file\n");
            return -1;
        }

        count -= writed;
    }

    return 0;
}

int record_wave(int fd, int channles, int sample_rate, int sample_length,
        int duration_time) {
    assert_die_if(fd < 0, "Invaild fd\n");
    assert_die_if(channles < 0, "Invaild channels\n");
    assert_die_if(sample_rate < 0, "Invaild sample rate\n");
    assert_die_if(sample_length < 0, "Invaild sample_length\n");
    assert_die_if(duration_time < 0, "Invaild time\n");

    int error = 0;

    error = prepare_wave_params(&wave_container, channles, sample_rate,
            sample_length, duration_time);
    if (error < 0) {
        LOGE("Failed to prepare wave params\n");
        goto error;
    }

    if (!hw_inited) {
        error = pcm_set_params(&pcm_container, LE_SHORT(sample_length), channles,
                sample_rate);
        if (error < 0) {
            LOGE("Failed to set hw params\n");
            goto error;
        }

        hw_inited = 1;
    }

#ifdef LOCAL_DEBUG
    snd_pcm_dump(pcm_container.pcm_handle, pcm_container.out_log);
#endif

    error = do_record_wave(&pcm_container, &wave_container, fd);
    if (error < 0) {
        LOGE("Failed to record\n");
        goto error;
    }

    snd_pcm_drain(pcm_container.pcm_handle);

    if (pcm_container.data_buf) {
        free(pcm_container.data_buf);
        pcm_container.data_buf = NULL;
    }

    return 0;

error:
    if (pcm_container.data_buf) {
        free(pcm_container.data_buf);
        pcm_container.data_buf = NULL;
    }

    return -1;
}

static int record_stream(int channels, int sample_rate, int sample_length,
        uint8_t** buffer) {
    assert_die_if(channels < 0, "Invaild channels\n");
    assert_die_if(sample_rate < 0, "Invaild sample rate\n");
    assert_die_if(sample_length < 0, "Invaild sample_length\n");

    int error = 0;

    if (!hw_inited) {
        error = pcm_set_params(&pcm_container, LE_SHORT(sample_length), channels,
                sample_rate);
        if (error < 0) {
            LOGE("Failed to set hw params\n");
            return -1;
        }

        hw_inited = 1;
    }

    error = pcm_read(&pcm_container, pcm_container.chunk_size);
    if (error != pcm_container.chunk_size) {
        LOGE("Failed to pcm write\n");
        return -1;
    }

    *buffer = pcm_container.data_buf;

    return pcm_container.chunk_bytes;
}

static int cancel_record(void) {
    int error = pcm_cancel(&pcm_container);

    if (pcm_container.data_buf) {
        free(pcm_container.data_buf);
        pcm_container.data_buf = NULL;
    }

    return error;
}

static int init(const char* snd_device) {
    assert_die_if(snd_device == NULL, "snd_device is NULL\n");

    int error = 0;

    pthread_mutex_lock(&init_lock);

    if (init_count++ == 0) {
        snd_output_stdio_attach(&pcm_container.out_log, stderr, 0);

        error = snd_pcm_open(&pcm_container.pcm_handle, snd_device,
                SND_PCM_STREAM_CAPTURE, 0);
        if (error < 0) {
            LOGE("Failed to snd_pcm_open %s: %s\n", snd_device, snd_strerror(error));
            goto error;
        }

        hw_inited = 0;
    }

    pthread_mutex_unlock(&init_lock);

    return 0;

error:
    init_count = 0;
    if (pcm_container.out_log)
        snd_output_close(pcm_container.out_log);

    pthread_mutex_unlock(&init_lock);

    return -1;
}

static int deinit(void) {
    pthread_mutex_lock(&init_lock);

    if (--init_count == 0) {
        hw_inited = 0;

        if (pcm_container.data_buf) {
            free(pcm_container.data_buf);
            pcm_container.data_buf = NULL;
        }

        snd_output_close(pcm_container.out_log);
        snd_pcm_close(pcm_container.pcm_handle);
    }

    pthread_mutex_unlock(&init_lock);

    return 0;
}

static struct wave_recorder this = {
        .init = init,
        .deinit = deinit,
        .record_wave = record_wave,
        .record_stream = record_stream,
        .cancel_record = cancel_record,
};

struct wave_recorder* get_wave_recorder(void) {
    return &this;
}
