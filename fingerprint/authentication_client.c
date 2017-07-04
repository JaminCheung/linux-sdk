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
#include <fingerprint/fingerprint.h>
#include <fingerprint/fingerprint_errorcode.h>
#include "hardware/fingerprint_device_proxy.h"
#include "authentication_client.h"

#define LOG_TAG "authentication_client"

static struct fingerprint_device_proxy* proxy;

static int on_enroll_result(struct client_monitor* base, int finger_id,
        int group_id, int remaining) {
    LOGD("%s called for authentication!\n", __FUNCTION__);

    return 0;
}

static int on_removed(struct client_monitor* base, int finger_id, int group_id) {
    LOGD("%s called for authentication!\n", __FUNCTION__);

    return 0;
}

static int on_enumeration_result(struct client_monitor* base, int finger_id,
        int group_id) {
    LOGD("%s called for authentication!\n", __FUNCTION__);

    return 0;
}

static int on_authenticated(struct client_monitor* base, int finger_id,
        int group_id) {
    int error = 0;
    int authenticated = finger_id != 0;

    if (!authenticated) {
        base->sender->on_authentication_failed(base->get_device_id(base));
    } else {
        struct fingerprint* fp = calloc(1, sizeof(struct fingerprint));
        fp->construct = construct_fingerprint;
        fp->destruct = destruct_fingerprint;
        fp->construct(fp, "NULL", group_id, finger_id, base->get_device_id(base));

        base->sender->on_authentication_successed(base->get_device_id(base), fp,
                base->get_user_id(base));;

        fp->destruct(fp);
        free(fp);
    }

    return error;
}

static int start(struct client_monitor* base) {
    int error = 0;

    struct authentication_client* this = to_this(&base,
            struct authentication_client, base);

    error = proxy->authenticate(this->op_id, base->get_group_id(base));
    if (error) {
        LOGW("Faild to start authentication: error %d\n", error);

        base->on_error(base, FINGERPRINT_ERROR_HW_UNAVAILABLE);

        return error;
    }

    return 0;
}

static int stop(struct client_monitor* base, int initiated_by_client) {
    int error = 0;

    error = proxy->stop_authentication();
    if (error) {
        LOGW("Failed to stop authentication: %d\n", error);
        return error;
    }

    return 0;
}

void construct_authentication_client(struct authentication_client* this,
        int64_t device_id, struct fingerprint_client_sender* sender,
        int user_id, int group_id, int64_t op_id, const char* owner) {
    this->base = calloc(1, sizeof(struct client_monitor));
    this->base->construct = construct_client_monitor;
    this->base->destruct = destruct_client_monitor;
    this->base->construct(this->base, device_id, sender, user_id, group_id,
            owner);
    this->base->start = start;
    this->base->stop = stop;
    this->base->on_enroll_result = on_enroll_result;
    this->base->on_removed = on_removed;
    this->base->on_enumeration_result = on_enumeration_result;
    this->base->on_authenticated = on_authenticated;

    this->op_id = op_id;

    if (proxy == NULL)
        proxy = get_fingerprint_device_proxy();
}

void destruct_authentication_client(struct authentication_client* this) {
    this->base->destruct(this->base);
    free(this->base);
    this->base = NULL;
}
