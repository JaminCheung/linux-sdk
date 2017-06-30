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

#include <string.h>

#include <utils/log.h>
#include <utils/assert.h>
#include "fingerprint_device_proxy.h"
#include "hardware/fingerprint_device.h"

#define LOG_TAG "fingerprint_device_proxy"

static struct fingerprint_device_callback* callback;
static struct fingerprint_device* device;

static void notify_key_store(const uint8_t* auth_token, const uint32_t auth_token_len) {
    //TODO: fill me
}

static void hardware_notify_callback(const fingerprint_msg_t* msg) {
    if (callback == NULL) {
        LOGE("Invalid callback\n");
        return;
    }

    const int64_t device_id = (unsigned long)device;

    switch(msg->type) {
    case FINGERPRINT_ERROR:
        LOGD("on_error(%d)", msg->data.error);
        callback->on_error(device_id, msg->data.error);
        break;

    case FINGERPRINT_ACQUIRED:
        LOGD("on_acquired(%d)", msg->data.acquired.acquired_info);
        callback->on_acquired(device_id, msg->data.acquired.acquired_info);
        break;

    case FINGERPRINT_AUTHENTICATED:
        LOGD("on_authenticated(fid=%d, gid=%d)",
                msg->data.authenticated.finger.fid,
                msg->data.authenticated.finger.gid);

        if (msg->data.authenticated.finger.fid != 0) {
            const uint8_t* hat = (const uint8_t*)(&msg->data.authenticated.hat);
            notify_key_store(hat, sizeof(msg->data.authenticated.hat));
        }

        callback->on_authenticated(device_id, msg->data.authenticated.finger.fid,
                msg->data.authenticated.finger.gid);
        break;

    case FINGERPRINT_TEMPLATE_ENROLLING:
        LOGD("on_enrollResult(fid=%d, gid=%d, rem=%d)",
                msg->data.enroll.finger.fid,
                msg->data.enroll.finger.gid,
                msg->data.enroll.samples_remaining);
        callback->on_enroll_result(device_id, msg->data.enroll.finger.fid,
                msg->data.enroll.finger.gid, msg->data.enroll.samples_remaining);
        break;

    case FINGERPRINT_TEMPLATE_REMOVED:
        LOGD("on_remove(fid=%d, gid=%d)",
                msg->data.removed.finger.fid,
                msg->data.removed.finger.gid);
        callback->on_removed(device_id, msg->data.removed.finger.fid,
                msg->data.removed.finger.gid);
        break;

    default:
        LOGE("Invaild msg type: %d\n", msg->type);
        return;
    }

}

static void init(struct fingerprint_device_callback* cb) {
    assert_die_if(cb == NULL, "Callback is NULL\n");

    callback = cb;
}

static int enroll(const char* token, uint32_t token_len, int group_id,
        int timeout) {
    LOGD("enroll(gid=%d, timeout=%d)\n", group_id, timeout);

    if (token_len != sizeof(hw_auth_token_t)) {
        LOGE("enroll(): invalid token size %du\n", token_len);
        return -1;
    }

    const hw_auth_token_t* auth_token = (const hw_auth_token_t*)token;

    return device->enroll(device, auth_token, group_id, timeout);
}

static uint64_t pre_enroll(void) {
    return device->pre_enroll(device);
}

static int post_enroll(void) {
    return device->post_enroll(device);
}

static int stop_enrollment(void) {
    LOGD("stop_enrollment()\n");
    return device->cancel(device);
}

static int authenticate(uint64_t session_id, int group_id) {
    LOGD("authenticate(sid=%llx, gid=%d)\n", session_id, group_id);
    return device->authenticate(device, session_id, group_id);
}

static int stop_authentication(void) {
    LOGD("stop_authentication()\n");
    return device->cancel(device);
}

static int remove_fingerprint(int finger_id, int group_id) {
    LOGD("remove(fid=%d, gid=%d)\n", finger_id, group_id);
    return device->remove(device, group_id, finger_id);
}

static uint64_t get_authenticator_id(void) {
    return device->get_authenticator_id(device);
}

static int set_active_group(int group_id, const uint8_t* path,
        uint32_t path_len) {
    if (path_len >= PATH_MAX) {
        LOGE("path length too long: %du\n", path_len);
        return -1;
    }

    char path_name[PATH_MAX];
    memcpy(path_name, path, path_len);
    path_name[path_len] = '\0';

    LOGD("setActiveGroup(%d, %s, %du)", group_id, path_name, path_len);

    return device->set_active_group(device, group_id, path_name);;
}

static int enumerate(void) {
    return 0;
}

static int stop_enumerate(void) {
    return 0;
}

static int64_t open_hal(void) {
    LOGD("Open hardware\n");

    int error = 0;

    device = get_fingerprint_device();
    error = device->set_notify(device, hardware_notify_callback);
    if (error < 0) {
        LOGE("Failed to call to set_notify(), error: %d\n", error);
        return 0;
    }

    if (device->notify != hardware_notify_callback)
        LOGE("device notify not set properly: %p != %p\n", device->notify,
                hardware_notify_callback);

    LOGD("Hardware successfully initialized\n");

    return (unsigned long)device;
}

static int close_hal(void) {
    LOGD("Close hardware\n");

    if (device == NULL) {
        LOGE("Invalid device\n");
        return -ENOSYS;
    }

    device = NULL;

    return 0;
}

struct fingerprint_device_proxy this = {
        .init = init,
        .enroll = enroll,
        .pre_enroll = pre_enroll,
        .post_enroll = post_enroll,
        .stop_enrollment = stop_enrollment,
        .enumerate = enumerate,
        .stop_enumerate = stop_enumerate,
        .authenticate = authenticate,
        .stop_authentication = stop_authentication,
        .remove_fingerprint = remove_fingerprint,
        .get_authenticator_id = get_authenticator_id,
        .set_active_group = set_active_group,
        .open_hal = open_hal,
        .close_hal = close_hal,
};

struct fingerprint_device_proxy* get_fingerprint_device_proxy(void) {
    return &this;
}
