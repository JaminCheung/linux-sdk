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

#ifndef FINGERPRINT_DEVICE_PROXY_H
#define FINGERPRINT_DEVICE_PROXY_H

#include <types.h>
#include "fingerprint_device_callback.h"

struct fingerprint_device_proxy {
    void (*init)(struct fingerprint_device_callback* callback);
    int (*enroll)(const char* token, uint32_t token_len, int group_id,
            int timeout);
    uint64_t (*pre_enroll)(void);
    int (*post_enroll)(void);
    int (*stop_enrollment)(void);
    int (*authenticate)(uint64_t session_id, int group_id);
    int (*stop_authentication)(void);
    int (*remove_fingerprint)(int finger_id, int group_id);
    uint64_t (*get_authenticator_id)(void);
    int (*set_active_group)(int group_id, const uint8_t *path, uint32_t path_len);
    int64_t (*open_hal)(void);
    int (*close_hal)(void);
};

struct fingerprint_device_proxy* get_fingerprint_device_proxy(void);

#endif /* FINGERPRINT_DEVICE_PROXY_H */
