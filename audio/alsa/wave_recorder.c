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

#include <math.h>

#include <utils/log.h>
#include <utils/assert.h>
#include <utils/common.h>
#include <utils/list.h>
#include <audio/alsa/wave_recorder.h>
#include "wave_pcm_common.h"

#define LOG_TAG "wave_recoder"

#define convert_prange1(val, min, max) \
    ceil((val) * ((max) - (min)) * 0.01 + (min))

struct mixer_element_id {
    snd_mixer_selem_id_t* sid;
    struct list_head node;
};

static WaveContainer wave_container;
static struct snd_pcm_container pcm_container;
static char card[64] = "default";
static pthread_mutex_t list_lock = PTHREAD_MUTEX_INITIALIZER;
static LIST_HEAD(element_id_list);

static void dump_control_id(void) {
    pthread_mutex_lock(&list_lock);

    LOGD("========================================\n");
    LOGD("Dump control id.\n");

    struct mixer_element_id* node = NULL;
    list_for_each_entry(node, &element_id_list, node) {
        LOGD("'%s',%i\n", snd_mixer_selem_id_get_name(node->sid),
                snd_mixer_selem_id_get_index(node->sid));
    }
    LOGD("========================================\n");

    pthread_mutex_unlock(&list_lock);
}

static struct mixer_element_id* get_elem_id_by_name(const char* name) {
    struct mixer_element_id* node = NULL;

    pthread_mutex_lock(&list_lock);

    list_for_each_entry(node, &element_id_list, node) {
        if (!strcmp(name, snd_mixer_selem_id_get_name(node->sid)))
            break;
    }

    pthread_mutex_unlock(&list_lock);

    return node;
}

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

static int record(struct snd_pcm_container* pcm_container,
        WaveContainer* wave_container, int fd) {
    int error = 0;
    uint64_t count;
    uint32_t writed;
    uint32_t frame_size;

    error = wave_write_header(fd, wave_container);
    if (error < 0) {
        LOGE("Failed to write wave header\n");
        return -1;
    }

    count = wave_container->chunk_header.length;
    while (count) {
        writed = (count <= pcm_container->chunk_bytes) ? count : pcm_container->chunk_bytes;
        frame_size = writed * 8 / pcm_container->bits_per_frame;

        if (wave_pcm_read(pcm_container, frame_size) != frame_size)
            break;

        if (write(fd, pcm_container->data_buf, writed) != writed) {
            LOGE("Failed to write wave file\n");
            return -1;
        }

        count -= writed;
    }

    return 0;
}

int record_file(const char* snd_device, int fd, int channles,
        int sample_rate, int sample_length, int duration_time) {
    assert_die_if(snd_device == NULL, "snd_device is NULL\n");
    assert_die_if(fd < 0, "Invaild fd\n");
    assert_die_if(channles < 0, "Invaild channels\n");
    assert_die_if(sample_rate < 0, "Invaild sample rate\n");
    assert_die_if(sample_length < 0, "Invaild sample_length\n");
    assert_die_if(duration_time < 0, "Invaild time\n");

    int error = 0;

    snd_output_stdio_attach(&pcm_container.out_log, stderr, 0);

    error = snd_pcm_open(&pcm_container.handle, snd_device,
            SND_PCM_STREAM_CAPTURE, 0);
    if (error < 0) {
        LOGE("Failed to snd_pcm_open %s: %s\n", snd_device, snd_strerror(error));
        goto error;
    }

    error = prepare_wave_params(&wave_container, channles, sample_rate,
            sample_length, duration_time);
    if (error < 0) {
        LOGE("Failed to prepare wave params\n");
        goto error;
    }

    error = wave_pcm_set_params(&pcm_container, &wave_container);
    if (error < 0) {
        LOGE("Failed to set hw params\n");
        goto error;
    }

#ifdef LOCAL_DEBUG
    snd_pcm_dump(pcm_container.handle, pcm_container.out_log);
#endif

    error = record(&pcm_container, &wave_container, fd);
    if (error < 0) {
        LOGE("Failed to record\n");
        goto error;
    }

    snd_pcm_drain(pcm_container.handle);

    free(pcm_container.data_buf);
    snd_output_close(pcm_container.out_log);
    snd_pcm_close(pcm_container.handle);

    return 0;

error:
    if (pcm_container.data_buf)
        free(pcm_container.data_buf);
    if (pcm_container.out_log)
        snd_output_close(pcm_container.out_log);
    if (pcm_container.handle)
        snd_pcm_close(pcm_container.handle);

    return -1;
}

static int get_volume(const char* name) {
    assert_die_if(name == NULL, "name is NULL\n");

    int volume = 0;


    return volume;
}

static int set_volume(const char* name, int volume) {
    assert_die_if(name == NULL, "name is NULL\n");

    int error = 0;
    int invalid = 0;
    long min, max;
    struct mixer_element_id* elem_id;
    snd_mixer_elem_t *elem = NULL;
    int val = 0;

    if (volume > 100)
        volume = 100;
    if (volume < 0)
        volume = 0;

    elem_id = get_elem_id_by_name(name);
    if (elem_id == NULL) {
        LOGE("Failed to get element id by name: %s\n", name);
        return -1;
    }

    elem = snd_mixer_find_selem(pcm_container.mixer_handle, elem_id->sid);
    if (elem == NULL) {
        LOGE("Failed to find control '%s',%i\n",
                snd_mixer_selem_id_get_name(elem_id->sid),
                snd_mixer_selem_id_get_index(elem_id->sid));
        return -1;
    }

    for (snd_mixer_selem_channel_id_t chn = 0; chn <= SND_MIXER_SCHN_LAST;
            chn++) {
        if (snd_mixer_selem_has_capture_channel(elem, chn)) {
            if (!snd_mixer_selem_has_capture_volume(elem))
                invalid = 1;

            if (snd_mixer_selem_get_capture_volume_range(elem, &min, &max) < 0)
                invalid = 1;

            val = (long)convert_prange1(volume, min, max);
            if (!invalid) {
                LOGD("Volume=%u, Max=%ld, Min=%ld\n", val, max, min);
                error = snd_mixer_selem_set_playback_volume(elem, chn, val);
                if (error) {
                    LOGE("Failed to set playback volume\n");
                    return -1;
                }
            }
        }
    }

    return 0;
}

static int init(void) {
    int error = 0;

    snd_mixer_elem_t *elem;

    error = snd_mixer_open(&pcm_container.mixer_handle, 0);
    if (error < 0) {
        LOGE("Mixer %s open error: %s", card, snd_strerror(error));
        return error;
    }

    error = snd_mixer_attach(pcm_container.mixer_handle, card);
    if (error < 0) {
        LOGE("Mixer attach %s error: %s", card, snd_strerror(error));
        goto error;
    }

    error = snd_mixer_selem_register(pcm_container.mixer_handle, NULL, NULL);
    if (error < 0) {
        LOGE("Mixer register error: %s", snd_strerror(error));
        goto error;
    }

    error = snd_mixer_load(pcm_container.mixer_handle);
    if (error < 0) {
        LOGE("Mixer load %s error: %s", card, snd_strerror(error));
        goto error;
    }

    pthread_mutex_lock(&list_lock);

    for (elem = snd_mixer_first_elem(pcm_container.mixer_handle); elem;
            elem = snd_mixer_elem_next(elem)) {
        if (!snd_mixer_selem_is_active(elem))
            continue;

        struct mixer_element_id* node = calloc(1,
                sizeof(struct mixer_element_id));

        node->sid = calloc(1, snd_mixer_selem_id_sizeof());

        snd_mixer_selem_get_id(elem, node->sid);

        list_add_tail(&node->node, &element_id_list);
    }
    pthread_mutex_unlock(&list_lock);

    dump_control_id();

    return 0;

error:
    snd_mixer_close(pcm_container.mixer_handle);
    return error;
}

static int deinit(void) {
    pthread_mutex_lock(&list_lock);

    struct mixer_element_id* node = NULL, *next_node = NULL;
    list_for_each_entry_safe(node, next_node, &element_id_list, node) {
        free(node->sid);
        free(node);
    }

    pthread_mutex_unlock(&list_lock);

    snd_mixer_close(pcm_container.mixer_handle);

    return 0;
}

static struct wave_recorder this = {
        .init = init,
        .deinit = deinit,
        .set_volume = set_volume,
        .get_volume = get_volume,
        .record_file = record_file,
};

struct wave_recorder* get_wave_recorder(void) {
    return &this;
}
