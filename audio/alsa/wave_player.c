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
#include <audio/alsa/wave_player.h>
#include "wave_pcm_common.h"

#define LOG_TAG "wave_player"

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

static uint32_t safe_read(int fd, void* buf, uint32_t count) {
    uint32_t read_sofar = 0;
    uint32_t readed;

    while (count > 0) {
        if ((readed = read(fd, buf, count)) == 0)
            break;

        if (readed < 0)
            return read_sofar > 0 ? read_sofar : readed;

        count -= readed;
        read_sofar += readed;
        buf = (char *)buf + readed;
    }

    return read_sofar;
}

static int play(struct snd_pcm_container* pcm_container,
        WaveContainer* wave_container, int fd) {
    uint64_t writed = 0;
    uint64_t count = LE_INT(wave_container->chunk_header.length);
    uint64_t c;
    int load = 0;
    int ret;

    while (writed < count) {
        do {
            c = count - writed;
            if (c > pcm_container->chunk_bytes)
                c = pcm_container->chunk_bytes;
            c -= load;

            if (c == 0)
                break;

            ret = safe_read(fd, pcm_container->data_buf + load, c);
            if (ret < 0) {
                LOGE("Failed to safe read\n");
                return -1;
            }

            if (ret == 0)
                break;

            load += ret;

        } while (load < pcm_container->chunk_bytes);

        load = load * 8 / pcm_container->bits_per_frame;
        ret = wave_pcm_write(pcm_container, load);
        if (ret != load)
            break;

        ret = ret * pcm_container->bits_per_frame / 8;
        writed += ret;
        load = 0;
    }

    return 0;
}

static int play_file(const char* snd_device, int fd) {
    assert_die_if(snd_device == NULL, "snd_device is NULL\n");
    assert_die_if(fd < 0, "Invaild fd\n");

    int error = 0;

    error = wave_read_header(fd, &wave_container);
    if (error < 0) {
        LOGE("Failed to read wave header\n");
        goto error;
    }

    snd_output_stdio_attach(&pcm_container.out_log, stderr, 0);

    error = snd_pcm_open(&pcm_container.handle, snd_device,
            SND_PCM_STREAM_PLAYBACK, 0);
    if (error < 0) {
        LOGE("Failed to snd_pcm_open %s: %s\n", snd_device, snd_strerror(error));
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

    error = play(&pcm_container, &wave_container, fd);
    if (error < 0) {
        LOGE("Failed to play\n");
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
        if (snd_mixer_selem_has_playback_channel(elem, chn)) {
            if (!snd_mixer_selem_has_playback_volume(elem))
                invalid = 1;

            if (snd_mixer_selem_get_playback_volume_range(elem, &min, &max) < 0)
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

static int mute(const char* name, int mute) {
    assert_die_if(name == NULL, "name is NULL\n");
    assert_die_if(mute != 0 && mute != 1, "Invalid mute\n");

    int error = 0;
    struct mixer_element_id* elem_id;
    snd_mixer_elem_t *elem = NULL;

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

        if (snd_mixer_selem_has_playback_channel(elem, chn)) {
            if (snd_mixer_selem_has_playback_switch(elem)) {
                LOGD("Mute=%d\n", mute);
                error = snd_mixer_selem_set_playback_switch(elem, chn, mute);
                if (error) {
                    LOGE("Failed to set playback mute\n");
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

static struct wave_player this = {
        .init = init,
        .deinit = deinit,
        .play_file = play_file,
        .set_volume = set_volume,
        .mute = mute,
};

struct wave_player* get_wave_player(void) {
    return &this;
}
