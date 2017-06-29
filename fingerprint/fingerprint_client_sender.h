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

#ifndef FINGERPRINT__CLIENT_SENDER_H
#define FINGERPRINT__CLIENT_SENDER_H

#include <fingerprint/fingerprint.h>

struct fingerprint_client_sender {
    void (*on_enroll_result)(int64_t device_id, int finger_id, int group_id,
            int remaining);
    void (*on_acquired)(int64_t device_id, int acquire_info);
    void (*on_authentication_successed)(int64_t device_id,
            struct fingerprint* fp, int user_id);
    void (*on_authentication_failed)(int64_t device_id);
    void (*on_error)(int64_t device_id, int error);
    void (*on_removed)(int64_t device_id, int finger_id, int group_id);
};

#endif /* FINGERPRINT__CLIENT_SENDER_H */
