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
#include <utils/list.h>
#include <lib/alsa/asoundlib.h>
#include <audio/alsa/mixer_controller.h>

#define LOG_TAG "mixer_controller"

#define convert_prange1(val, min, max) \
    ceil((val) * ((max) - (min)) * 0.01 + (min))

struct mixer_element_id {
    snd_mixer_selem_id_t* sid;
    struct list_head node;
};

struct volume_ops {
    int (*get_range)(snd_mixer_elem_t *elem, long *min, long *max);
    int (*get)(snd_mixer_elem_t *elem, snd_mixer_selem_channel_id_t c,
           long *value);
    int (*set)(snd_mixer_elem_t *elem, snd_mixer_selem_channel_id_t c,
           long value);
};

struct volume_ops_set {
    int (*has_volume)(snd_mixer_elem_t *elem);
    struct volume_ops v;
};

static const struct volume_ops_set vol_ops[2] = {
    [PLAYBACK] = {
        .has_volume = snd_mixer_selem_has_playback_volume,
        .v = {
                snd_mixer_selem_get_playback_volume_range,
                snd_mixer_selem_get_playback_volume,
                snd_mixer_selem_set_playback_volume ,
        },
    },

    [CAPTURE] = {
        .has_volume = snd_mixer_selem_has_capture_volume,
        .v = {
                snd_mixer_selem_get_capture_volume_range,
                snd_mixer_selem_get_capture_volume,
                snd_mixer_selem_set_capture_volume,
        },
    },
};

static snd_mixer_t *handle;
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

static int convert_prange(long val, long min, long max) {
    long range = max - min;
    int tmp;

    if (range == 0)
        return 0;
    val -= min;
    tmp = rint((double)val/(double)range * 100);
    return tmp;
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

static int set_volume_simple(snd_mixer_elem_t* elem,
        snd_mixer_selem_channel_id_t chn, int dir, int volume) {
    int error = 0;
    int invalid = 0;
    int val = 0;
    long min, max;

    if (!vol_ops[dir].has_volume(elem))
        invalid = 1;

    if (vol_ops[dir].v.get_range(elem, &min, &max) < 0)
        invalid = 1;

    val = (long)convert_prange1(volume, min, max);
    if (!invalid) {
        error = vol_ops[dir].v.set(elem, chn, val);
        if (error) {
            LOGE("Failed to set volume\n");
            return -1;
        }
    }

    return 0;
}

static int get_volume_simple(snd_mixer_elem_t* elem,
        snd_mixer_selem_channel_id_t chn, int dir, long *volume) {
    int error = 0;
    int invalid = 0;
    long val = 0;
    long min, max;

    if (!vol_ops[dir].has_volume(elem))
        invalid = 1;

    if (vol_ops[dir].v.get_range(elem, &min, &max) < 0)
        invalid = 1;

    if (!invalid) {
        error = vol_ops[dir].v.get(elem, chn, &val);
        if (error) {
            LOGE("Failed to set playback volume\n");
            return -1;
        }

        *volume = (long)convert_prange(val, min, max);
    }

    return 0;
}

static int set_volume(const char* name, int dir, int volume) {
    assert_die_if(name == NULL, "name is NULL\n");

    int error = 0;
    struct mixer_element_id* elem_id;
    snd_mixer_elem_t *elem = NULL;

    if (volume > 100)
        volume = 100;
    if (volume < 0)
        volume = 0;

    elem_id = get_elem_id_by_name(name);
    if (elem_id == NULL) {
        LOGE("Failed to get element id by name: %s\n", name);
        return -1;
    }

    elem = snd_mixer_find_selem(handle, elem_id->sid);
    if (elem == NULL) {
        LOGE("Failed to find control '%s',%i\n",
                snd_mixer_selem_id_get_name(elem_id->sid),
                snd_mixer_selem_id_get_index(elem_id->sid));
        return -1;
    }

    for (snd_mixer_selem_channel_id_t chn = 0; chn <= SND_MIXER_SCHN_LAST;
            chn++) {

        if (dir == PLAYBACK && snd_mixer_selem_has_playback_channel(elem, chn)) {
            error = set_volume_simple(elem, chn, PLAYBACK, volume);
            if (error < 0) {
                LOGE("Failed to set playback volume\n");
                return -1;
            }
        }

        if (dir == CAPTURE && snd_mixer_selem_has_capture_channel(elem, chn)) {
            error = set_volume_simple(elem, chn, CAPTURE, volume);
            if (error < 0) {
                LOGE("Failed to set capture volume\n");
                return -1;
            }
        }
    }

    return 0;
}

static int get_volume(const char* name, int dir) {
    assert_die_if(name == NULL, "name is NULL\n");

    int error = 0;
    long volume = 0;;
    struct mixer_element_id* elem_id;
    snd_mixer_elem_t *elem = NULL;

    elem_id = get_elem_id_by_name(name);
    if (elem_id == NULL) {
        LOGE("Failed to get element id by name: %s\n", name);
        return -1;
    }

    elem = snd_mixer_find_selem(handle, elem_id->sid);
    if (elem == NULL) {
        LOGE("Failed to find control '%s',%i\n",
                snd_mixer_selem_id_get_name(elem_id->sid),
                snd_mixer_selem_id_get_index(elem_id->sid));
        return -1;
    }

    for (snd_mixer_selem_channel_id_t chn = 0; chn <= SND_MIXER_SCHN_LAST;
            chn++) {

        if (dir == PLAYBACK && snd_mixer_selem_has_playback_channel(elem, chn)) {
            error = get_volume_simple(elem, chn, PLAYBACK, &volume);
            if (error < 0) {
                LOGE("Failed to set playback volume\n");
                return -1;
            }
        }

        if (dir == CAPTURE && snd_mixer_selem_has_capture_channel(elem, chn)) {
            error = get_volume_simple(elem, chn, CAPTURE, &volume);
            if (error < 0) {
                LOGE("Failed to set capture volume\n");
                return -1;
            }
        }
    }

    return volume;
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

    elem = snd_mixer_find_selem(handle, elem_id->sid);
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

    error = snd_mixer_open(&handle, 0);
    if (error < 0) {
        LOGE("Mixer %s open error: %s", card, snd_strerror(error));
        return error;
    }

    error = snd_mixer_attach(handle, card);
    if (error < 0) {
        LOGE("Mixer attach %s error: %s", card, snd_strerror(error));
        goto error;
    }

    error = snd_mixer_selem_register(handle, NULL, NULL);
    if (error < 0) {
        LOGE("Mixer register error: %s", snd_strerror(error));
        goto error;
    }

    error = snd_mixer_load(handle);
    if (error < 0) {
        LOGE("Mixer load %s error: %s", card, snd_strerror(error));
        goto error;
    }

    pthread_mutex_lock(&list_lock);

    for (elem = snd_mixer_first_elem(handle); elem;
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
    snd_mixer_close(handle);
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

    snd_mixer_close(handle);

    return 0;
}

static struct mixer_controller this = {
        .init = init,
        .deinit = deinit,
        .set_volume = set_volume,
        .get_volume = get_volume,
        .mute = mute,
};

struct mixer_controller* get_mixer_controller(void) {
    return &this;
}
