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

#include <stdlib.h>

#include <utils/log.h>
#include <utils/list.h>
#include <utils/assert.h>
#include <utils/file_ops.h>
#include <fingerprint2/fingerprint_manager.h>

#include "goodix_interface.h"
#include "ma_fingerprint.h"

#define LOG_TAG "fingerprint_manager"

#define GOODIX_DEV        "/dev/goodix_fp"
#define MICROARRAY_DEV     "/dev/madev0"

enum {
    VENDOR_GOODIX,
    VENDOR_MICROARRAY,
    VENDOR_UNKNOWN,
};

static int inited;
static int sensor_vendor = -1;
static pthread_mutex_t listener_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t init_lock = PTHREAD_MUTEX_INITIALIZER;

static LIST_HEAD(listener_list);

struct listener {
    fingerprint_event_listener_t cb;
    struct list_head node;
};

static void goodix_fp_callback(int msg, int percent, int finger_id) {
    struct listener* l;

    pthread_mutex_lock(&listener_lock);
    list_for_each_entry(l, &listener_list, node)
        l->cb(msg, percent, finger_id);

    pthread_mutex_unlock(&listener_lock);
}

static int init(void) {
    pthread_mutex_lock(&init_lock);
    if (inited == 1) {
        LOGE("fingerprint manager already init\n");
        pthread_mutex_unlock(&init_lock);
        return -1;
    }

    int error = 0;

    if (file_exist(MICROARRAY_DEV) == 0)
        sensor_vendor = VENDOR_MICROARRAY;
    else if (file_exist(GOODIX_DEV) == 0)
        sensor_vendor = VENDOR_GOODIX;
    else
        assert_die_if(1, "Have not found any fingerprint sensor!\n");


    if (sensor_vendor == VENDOR_GOODIX) {
        error = fingerprint_init(goodix_fp_callback);

    } else if (sensor_vendor == VENDOR_MICROARRAY) {
        assert_die_if(1, "please implement me.");
    }

    inited = 1;

    pthread_mutex_unlock(&init_lock);

    return error;
}

static int deinit(void) {
    pthread_mutex_lock(&init_lock);

    if (inited == 0) {
        LOGE("fingerprint manager already deinit\n");
        pthread_mutex_unlock(&init_lock);
        return -1;
    }
    inited = 0;

    int error = 0;

    if (sensor_vendor == VENDOR_GOODIX) {
        error = fingerprint_destroy();

    } else if (sensor_vendor == VENDOR_MICROARRAY) {
        assert_die_if(1, "please implement me.\n");
    }

    sensor_vendor = VENDOR_UNKNOWN;

    pthread_mutex_unlock(&init_lock);

    return error;
}

static int enroll(void) {
    assert_die_if(sensor_vendor == VENDOR_UNKNOWN,
            "Have not found any fingerprint sensor!\n");

    int error = 0;

    if (sensor_vendor == VENDOR_GOODIX) {
        error = fingerprint_enroll();

    } else if (sensor_vendor == VENDOR_MICROARRAY) {
        assert_die_if(1, "please implement me.\n");
    }

    return error;
}

static int authenticate(void) {
    assert_die_if(sensor_vendor == VENDOR_UNKNOWN,
            "Have not found any fingerprint sensor!\n");

    int error = 0;

    if (sensor_vendor == VENDOR_GOODIX) {
        error = fingerprint_authenticate();

    } else if (sensor_vendor == VENDOR_MICROARRAY) {
        assert_die_if(1, "please implement me.\n");
    }

    return error;
}

static int delete(int finger_id, delete_type_t type) {
    assert_die_if(sensor_vendor == VENDOR_UNKNOWN,
            "Have not found any fingerprint sensor!\n");

    int error = 0;

    if (sensor_vendor == VENDOR_GOODIX) {
        error = fingerprint_delete(finger_id, type);

    } else if (sensor_vendor == VENDOR_MICROARRAY) {
        assert_die_if(1, "please implement me.\n");
    }

    return error;
}

static int cancel(void) {
    assert_die_if(sensor_vendor == VENDOR_UNKNOWN,
            "Have not found any fingerprint sensor!\n");

    int error = 0;

    if (sensor_vendor == VENDOR_GOODIX) {
        error = fingerprint_cancel();

    } else if (sensor_vendor == VENDOR_MICROARRAY) {
        assert_die_if(1, "please implement me.\n");
    }

    return error;
}

static int reset(void) {
    assert_die_if(sensor_vendor == VENDOR_UNKNOWN,
            "Have not found any fingerprint sensor!\n");

    int error = 0;

    if (sensor_vendor == VENDOR_GOODIX) {
        error = fingerprint_reset();

    } else if (sensor_vendor == VENDOR_MICROARRAY) {
        assert_die_if(1, "please implement me.\n");
    }

    return error;
}

static int set_enroll_timeout(int timeout_ms) {
    assert_die_if(sensor_vendor == VENDOR_UNKNOWN,
            "Have not found any fingerprint sensor!\n");

    int error = 0;

    if (sensor_vendor == VENDOR_GOODIX) {
        error = fingerprint_set_enroll_timeout(timeout_ms);

    } else if (sensor_vendor == VENDOR_MICROARRAY) {
        assert_die_if(1, "please implement me.\n");
    }

    return error;
}

static int set_authenticate_timeout(int timeout_ms) {
    assert_die_if(sensor_vendor == VENDOR_UNKNOWN,
            "Have not found any fingerprint sensor!\n");

    int error = 0;

    if (sensor_vendor == VENDOR_GOODIX) {
        error = fingerprint_set_authenticate_timeout(timeout_ms);

    } else if (sensor_vendor == VENDOR_MICROARRAY) {
        assert_die_if(1, "please implement me.\n");
    }

    return error;
}

static int post_power_on(void) {
    assert_die_if(sensor_vendor == VENDOR_UNKNOWN,
            "Have not found any fingerprint sensor!\n");

    int error = 0;

    if (sensor_vendor == VENDOR_GOODIX) {
        error = fingerprint_post_power_on();

    } else if (sensor_vendor == VENDOR_MICROARRAY) {
        assert_die_if(1, "please implement me.\n");
    }

    return error;
}

static int post_power_off(void) {
    assert_die_if(sensor_vendor == VENDOR_UNKNOWN,
            "Have not found any fingerprint sensor!\n");

    int error = 0;

    if (sensor_vendor == VENDOR_GOODIX) {
        error = fingerprint_post_power_off();

    } else if (sensor_vendor == VENDOR_MICROARRAY) {
        assert_die_if(1, "please implement me.\n");
    }

    return error;
}

static void register_event_listener(fingerprint_event_listener_t listener) {
    assert_die_if(sensor_vendor == VENDOR_UNKNOWN,
            "Have not found any fingerprint sensor!\n");

    assert_die_if(listener == NULL, "listener is NULL\n");

    struct listener *l = NULL;

    pthread_mutex_lock(&listener_lock);

    list_for_each_entry(l, &listener_list, node) {
        if (l->cb == listener) {
            pthread_mutex_unlock(&listener_lock);
            return;
        }
    }

    l = malloc(sizeof(struct listener));
    l->cb = listener;
    list_add_tail(&l->node, &listener_list);

    pthread_mutex_unlock(&listener_lock);
}

static void unregister_event_listener(fingerprint_event_listener_t listener) {
    assert_die_if(sensor_vendor == VENDOR_UNKNOWN,
            "Have not found any fingerprint sensor!\n");

    assert_die_if(listener == NULL, "listener is NULL\n");

    struct listener *l, *nl;

    pthread_mutex_lock(&listener_lock);

    list_for_each_entry_safe(l, nl, &listener_list, node) {
        if (l->cb == listener) {
            list_del(&l->node);

            free(l);

            pthread_mutex_unlock(&listener_lock);

            return;
        }
    }

    pthread_mutex_unlock(&listener_lock);
}

struct fingerprint_manager this = {
        .init = init,
        .deinit = deinit,
        .register_event_listener = register_event_listener,
        .unregister_event_listener = unregister_event_listener,
        .enroll = enroll,
        .authenticate = authenticate,
        .delete = delete,
        .cancel = cancel,
        .reset = reset,
        .set_enroll_timeout = set_enroll_timeout,
        .set_authenticate_timeout = set_authenticate_timeout,
        .post_power_off = post_power_off,
        .post_power_on = post_power_on,
};

struct fingerprint_manager* get_fingerprint_manager(void) {
    return &this;
}
