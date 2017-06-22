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
#include <fingerprint/fingerprint_manager.h>

#define LOG_TAG "fingerprint_manager"



static void cancel_enrollment(void) {

}

static void cancel_authentication(void) {

}

static const char* get_error_string(int error_code) {
    switch (error_code) {
    case FINGERPRINT_ERROR_UNABLE_TO_PROCESS:
        return NULL;

    case FINGERPRINT_ERROR_HW_UNAVAILABLE:
        return NULL;

    case FINGERPRINT_ERROR_NO_SPACE:
        return NULL;

    case FINGERPRINT_ERROR_TIMEOUT:
        return NULL;

    case FINGERPRINT_ERROR_CANCELED:
        return NULL;

    case FINGERPRINT_ERROR_LOCKOUT:
        return NULL;

    default:
        return NULL;
    }
}

static const char* get_acquired_string(int acquire_info) {
    switch(acquire_info) {
    case FINGERPRINT_ACQUIRED_GOOD:
        return NULL;
    case FINGERPRINT_ACQUIRED_PARTIAL:
        return NULL;
    case FINGERPRINT_ACQUIRED_INSUFFICIENT:
        return NULL;
    case FINGERPRINT_ACQUIRED_IMAGER_DIRTY:
        return NULL;
    case FINGERPRINT_ACQUIRED_TOO_SLOW:
        return NULL;
    case FINGERPRINT_ACQUIRED_TOO_FAST:
        return NULL;
    default:
        return NULL;
    }
}

/* ================================ Public API ============================== */

static int is_hardware_detected(void) {
    return 0;
}

static int has_enrolled_fingerprints(void) {
    return 0;
}

static void authenticate(struct authentication_callback *callback) {

}

static void enroll(char* token, int len, struct enrollment_callback *callback) {

}

static void pre_enroll(void) {

}

static void post_enroll(void) {

}

static void remove_fingerprint(struct fingerprint *fp, struct remove_callback* callabck) {

}

static void rename_fingerprint(int fp_id, const char* new_name) {

}

static void get_enrolled_fingerprints(void) {

}

static uint64_t get_authenticator_id(void) {
    return 0;
}

static void reset_timeout(char* token, int len) {

}

static void add_lockout_reset_callback(struct lockout_reset_callback* callback) {

}

/* ============================== Public API end ============================ */

static struct fingerprint_manager this = {
        .is_hardware_detected = is_hardware_detected,
        .has_enrolled_fingerprints = has_enrolled_fingerprints,
        .authenticate = authenticate,
        .enroll = enroll,
        .pre_enroll = pre_enroll,
        .post_enroll = post_enroll,
        .remove_fingerprint = remove_fingerprint,
        .rename_fingerprint = rename_fingerprint,
        .get_enrolled_fingerprints = get_enrolled_fingerprints,
        .get_authenticator_id = get_authenticator_id,
        .reset_timeout = reset_timeout,
        .add_lockout_reset_callback = add_lockout_reset_callback,
};

struct fingerprint_manager* get_fingerprint_manager(void) {
    return &this;
}
