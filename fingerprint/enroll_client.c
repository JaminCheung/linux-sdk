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
#include <string.h>

#include <utils/log.h>
#include <utils/linux.h>
#include <fingerprint/fingerprint_errorcode.h>
#include "fingerprint_utils.h"
#include "hardware/fingerprint_device_proxy.h"
#include "enroll_client.h"

#define LOG_TAG "enroll_client"

#define MS_PER_SEC 1000
#define ENROLLMENT_TIMEOUT_MS (60 * 1000)

static struct fingerprint_utils* utils;
static struct fingerprint_device_proxy* proxy;

static int on_enroll_result(struct client_monitor* base, int finger_id,
        int group_id, int remaining) {
    if (group_id != base->get_group_id(base)) {
        LOGW("group_id != get_group_id(), group_id=%d get_group_id=%d\n",
                group_id, base->get_group_id(base));
    }

    if (remaining == 0)
        utils->add_fingerprint_for_user(finger_id, base->user_id);

    struct enroll_client* this = to_this(&base, struct enroll_client, base);

    return this->send_enroll_result(this, finger_id, group_id, remaining);
}

static int on_removed(struct client_monitor* base, int finger_id, int group_id) {
    LOGD("%s called for enroll!\n", __FUNCTION__);

    return 0;
}

static int on_enumeration_result(struct client_monitor* base, int finger_id,
        int group_id) {
    LOGD("%s called for enroll!\n", __FUNCTION__);

    return 0;
}

static int on_authenticated(struct client_monitor* base, int finger_id,
        int group_id) {
    LOGD("%s called for enroll!\n", __FUNCTION__);

    return 0;
}

static int start(struct client_monitor* base) {
    int error = 0;
    struct enroll_client* this = to_this(&base, struct enroll_client, base);

    int timeout = (int) (ENROLLMENT_TIMEOUT_MS / MS_PER_SEC);

    error = proxy->enroll(this->token, this->token_len,
            base->get_group_id(base), timeout);
    if (error) {
        LOGE("Failedl to start enroll: %d\n", error);
        base->on_error(base, FINGERPRINT_ERROR_HW_UNAVAILABLE);

        return error;
    }

    return 0;
}

static int stop(struct client_monitor* base, int initiated_by_client) {
    int error = 0;

    error = proxy->stop_enrollment();
    if (error) {
        LOGE("Failed to stop enrollment: %d\n", error);
        return error;
    }

    if (initiated_by_client)
        base->on_error(base, FINGERPRINT_ERROR_CANCELED);

    return 0;
}

static int send_enroll_resutl(struct enroll_client* this, int finger_id,
        int group_id, int remaining) {
    if (this->base->sender == NULL)
        return - 1;

    this->base->sender->on_enroll_result(this->base->get_device_id(this->base),
            finger_id, group_id, remaining);

    return remaining == 0;

}

void construct_enroll_client(struct enroll_client* this, int64_t device_id,
        struct fingerprint_client_sender* sender, int user_id, int group_id,
        char* token, int token_len) {

    this->base = calloc(1, sizeof(struct client_monitor));
    this->base->construct = construct_client_monitor;
    this->base->destruct = destruct_client_monitor;
    this->base->construct(this->base, device_id, sender, user_id, group_id);
    this->base->start = start;
    this->base->stop = stop;
    this->base->on_enroll_result = on_enroll_result;
    this->base->on_removed = on_removed;
    this->base->on_enumeration_result = on_enumeration_result;
    this->base->on_authenticated = on_authenticated;

    this->token = calloc(token_len, sizeof(char));
    memcpy(this->token, token, token_len);
    this->token_len = token_len;
    this->send_enroll_result = send_enroll_resutl;

    if (utils == NULL)
        utils = get_fingerprint_utils();
    if (proxy == NULL)
        proxy = get_fingerprint_device_proxy();
}

void destruct_enroll_client(struct enroll_client* this) {
    this->base->destruct(this->base);
    free(this->base);
    this->base = NULL;
}
