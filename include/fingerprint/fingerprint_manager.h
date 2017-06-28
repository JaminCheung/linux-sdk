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

#ifndef FINGERPRINT_MANAGER_H
#define FINGERPRINT_MANAGER_H

#include <types.h>
#include <fingerprint/fingerprint.h>
#include <fingerprint/fingerprint_list.h>
#include <fingerprint/authentication_result.h>

struct enrollment_callback {
    void (*on_enrollment_error)(int error_code, const char* error_string);
    void (*on_enrollment_help)(int help_code, const char* help_string);
    void (*on_enrollment_progress)(int remaining);
};

struct removal_callback {
    void (*on_removal_error)(struct fingerprint *fp, int error_code,
            const char* error_string);
    void (*on_removal_successed)(struct fingerprint *fp);
};

struct authentication_callback {
    void (*on_authentication_error)(int error_code, const char* error_string);
    void (*on_authentication_help)(int help_code, const char* help_string);
    void (*on_authentication_successed)(struct authentication_result *result);
    void (*on_authentication_failed)(void);
    void (*on_authentication_acquired)(int acquire_info);
};

struct lockout_reset_callback {
    void (*on_lockout_reset)(void);
};

struct fingerprint_manager {
    int (*init)(void);
    int (*deinit)(void);
    int (*is_hardware_detected)(void);
    int (*has_enrolled_fingerprints)(int user_id);
    void (*authenticate)(struct authentication_callback *callback, int flag, int user_id);
    void (*enroll)(char* token, int len, struct enrollment_callback *callback);
    int64_t (*pre_enroll)(void);
    int (*post_enroll)(void);
    void (*set_active_user)(int user_id);
    void (*remove_fingerprint)(struct fingerprint *fp, struct removal_callback* callabck);
    void (*rename_fingerprint)(int fp_id, const char* new_name);

    struct fingerprint_list* (*get_enrolled_fingerprints)(int user_id);

    int64_t (*get_authenticator_id)(void);
    void (*reset_timeout)(char* token, int len);
    void (*add_lockout_reset_callback)(struct lockout_reset_callback* callback);
};

struct fingerprint_manager* get_fingerprint_manager(void);

#endif /* FINGERPRINT_MANAGER_H */
