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

#include <errno.h>
#include <malloc.h>
#include <string.h>

#include <utils/log.h>

#include "android/hardware.h"
#include "fingerprint_device.h"

#define LOG_TAG "fingerprint_device_demo"

static int fingerprint_close(hw_device_t *dev) {
    if (dev) {
        free(dev);
        return 0;
    } else {
        return -1;
    }
}

static uint64_t fingerprint_pre_enroll(struct fingerprint_device *dev) {
    return FINGERPRINT_ERROR;
}

static int fingerprint_enroll(struct fingerprint_device *dev,
        const hw_auth_token_t  *hat, uint32_t  gid, uint32_t timeout_sec) {
    return FINGERPRINT_ERROR;
}

static uint64_t fingerprint_get_auth_id(struct fingerprint_device *dev) {
    return FINGERPRINT_ERROR;
}

static int fingerprint_cancel(struct fingerprint_device *dev) {
    return FINGERPRINT_ERROR;
}

static int fingerprint_remove(struct fingerprint_device *dev,
        uint32_t gid, uint32_t fid) {
    return FINGERPRINT_ERROR;
}

static int fingerprint_set_active_group(struct fingerprint_device *dev,
        uint32_t gid, const char *store_path) {
    return FINGERPRINT_ERROR;
}

static int fingerprint_authenticate(struct fingerprint_device *dev,
        uint64_t operation_id, uint32_t gid) {
    return FINGERPRINT_ERROR;
}

static int set_notify_callback(struct fingerprint_device *dev,
                                fingerprint_notify_t notify) {
    /* Decorate with locks */
    dev->notify = notify;
    return FINGERPRINT_ERROR;
}

static int fingerprint_open(const hw_module_t* module, const char *id,
                            hw_device_t** device)
{
    if (device == NULL) {
        LOGE("NULL device on open");
        return -EINVAL;
    }

    fingerprint_device_t *dev = malloc(sizeof(fingerprint_device_t));
    memset(dev, 0, sizeof(fingerprint_device_t));

    dev->common.tag = HARDWARE_DEVICE_TAG;
    dev->common.version = FINGERPRINT_MODULE_API_VERSION_2_0;
    dev->common.module = (struct hw_module_t*) module;
    dev->common.close = fingerprint_close;

    dev->pre_enroll = fingerprint_pre_enroll;
    dev->enroll = fingerprint_enroll;
    dev->get_authenticator_id = fingerprint_get_auth_id;
    dev->cancel = fingerprint_cancel;
    dev->remove = fingerprint_remove;
    dev->set_active_group = fingerprint_set_active_group;
    dev->authenticate = fingerprint_authenticate;
    dev->set_notify = set_notify_callback;
    dev->notify = NULL;

    *device = (hw_device_t*) dev;
    return 0;
}

static struct hw_module_methods_t fingerprint_module_methods = {
    .open = fingerprint_open,
};

fingerprint_module_t HAL_MODULE_INFO_SYM = {
    .common = {
        .tag                = HARDWARE_MODULE_TAG,
        .module_api_version = FINGERPRINT_MODULE_API_VERSION_2_0,
        .hal_api_version    = HARDWARE_HAL_API_VERSION,
        .id                 = FINGERPRINT_HARDWARE_MODULE_ID,
        .name               = "Demo Fingerprint HAL",
        .author             = "The Android Open Source Project",
        .methods            = &fingerprint_module_methods,
    },
};
