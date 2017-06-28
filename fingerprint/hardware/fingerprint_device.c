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

#include "fingerprint_device.h"

#define LOG_TAG "fingerprint_device"

static struct fingerprint_device this;

static uint64_t fingerprint_pre_enroll(struct fingerprint_device *this) {
    return FINGERPRINT_ERROR;
}

static int fingerprint_enroll(struct fingerprint_device *this,
        const hw_auth_token_t *hat, uint32_t gid, uint32_t timeout_sec) {
    return FINGERPRINT_ERROR;
}

static uint64_t fingerprint_get_auth_id(struct fingerprint_device *this) {
    return FINGERPRINT_ERROR;
}

static int fingerprint_cancel(struct fingerprint_device *this) {
    return FINGERPRINT_ERROR;
}

static int fingerprint_remove(struct fingerprint_device *this, uint32_t gid,
        uint32_t fid) {
    return FINGERPRINT_ERROR;
}

static int fingerprint_set_active_group(struct fingerprint_device *this,
                                        uint32_t gid, const char *store_path) {
    return FINGERPRINT_ERROR;
}

static int fingerprint_authenticate(struct fingerprint_device *this,
                                    uint64_t operation_id, uint32_t gid) {
    return FINGERPRINT_ERROR;
}

static int set_notify_callback(struct fingerprint_device *this,
                                fingerprint_notify_t notify) {
    /* Decorate with locks */
    this->notify = notify;
    return FINGERPRINT_ERROR;
}

static struct fingerprint_device this = {
        .set_notify = set_notify_callback,
        .pre_enroll = fingerprint_pre_enroll,
        .enroll = fingerprint_enroll,
        .post_enroll = NULL,
        .get_authenticator_id = fingerprint_get_auth_id,
        .cancel = fingerprint_cancel,
        .enumerate = NULL,
        .remove = fingerprint_remove,
        .set_active_group = fingerprint_set_active_group,
        .authenticate = fingerprint_authenticate,
        .notify = NULL,
};

struct fingerprint_device* get_fingerprint_device(void) {
    return &this;
}
