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
#include <utils/common.h>
#include <fingerprint/fingerprint_manager.h>
#include <fingerprint/fingerprint_errorcode.h>

#define LOG_TAG "fingerprint_manager"

static const char* error_strings[] = {
        [FINGERPRINT_ERROR_HW_UNAVAILABLE] = "Fingerprint hardware not available.",
        [FINGERPRINT_ERROR_UNABLE_TO_PROCESS] = "Try again.",
        [FINGERPRINT_ERROR_TIMEOUT] = "Fingerprint time out reached. Try again.",
        [FINGERPRINT_ERROR_NO_SPACE] = "Fingerprint can\'t be stored. Please remove an existing fingerprint.",
        [FINGERPRINT_ERROR_CANCELED] = "Fingerprint operation canceled.",
        [FINGERPRINT_ERROR_UNABLE_TO_REMOVE] = "Fingerprint can\'t be removed",
        [FINGERPRINT_ERROR_LOCKOUT] = "Too many attempts. Try again later.",
};

static const char* acquired_strings[] = {
        [FINGERPRINT_ACQUIRED_PARTIAL] = "Partial fingerprint detected. Please try again.",
        [FINGERPRINT_ACQUIRED_INSUFFICIENT] = "Couldn\'t process fingerprint. Please try again.",
        [FINGERPRINT_ACQUIRED_IMAGER_DIRTY] = "Fingerprint sensor is dirty. Please clean and try again.",
        [FINGERPRINT_ACQUIRED_TOO_SLOW] = "Finger moved too slow. Please try again.",
        [FINGERPRINT_ACQUIRED_TOO_FAST] = "Finger moved too fast. Please try again.",
};

static void cancel_enrollment(void) {

}

static void cancel_authentication(void) {

}

static const char* get_error_string(int error_code) {
    switch (error_code) {
    case FINGERPRINT_ERROR_UNABLE_TO_PROCESS:
        return error_strings[FINGERPRINT_ERROR_UNABLE_TO_PROCESS];

    case FINGERPRINT_ERROR_HW_UNAVAILABLE:
        return error_strings[FINGERPRINT_ERROR_HW_UNAVAILABLE];

    case FINGERPRINT_ERROR_NO_SPACE:
        return error_strings[FINGERPRINT_ERROR_NO_SPACE];

    case FINGERPRINT_ERROR_TIMEOUT:
        return error_strings[FINGERPRINT_ERROR_TIMEOUT];

    case FINGERPRINT_ERROR_CANCELED:
        return error_strings[FINGERPRINT_ERROR_CANCELED];

    case FINGERPRINT_ERROR_LOCKOUT:
        return error_strings[FINGERPRINT_ERROR_LOCKOUT];

    default:
        return NULL;
    }
}

static const char* get_acquired_string(int acquire_info) {
    switch(acquire_info) {
    case FINGERPRINT_ACQUIRED_GOOD:
        return NULL;

    case FINGERPRINT_ACQUIRED_PARTIAL:
        return acquired_strings[FINGERPRINT_ACQUIRED_PARTIAL];

    case FINGERPRINT_ACQUIRED_INSUFFICIENT:
        return acquired_strings[FINGERPRINT_ACQUIRED_INSUFFICIENT];

    case FINGERPRINT_ACQUIRED_IMAGER_DIRTY:
        return acquired_strings[FINGERPRINT_ACQUIRED_IMAGER_DIRTY];

    case FINGERPRINT_ACQUIRED_TOO_SLOW:
        return acquired_strings[FINGERPRINT_ACQUIRED_TOO_SLOW];

    case FINGERPRINT_ACQUIRED_TOO_FAST:
        return acquired_strings[FINGERPRINT_ACQUIRED_TOO_FAST];

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

static struct fingerprint_list* get_enrolled_fingerprints(void) {
    return NULL;
}

static uint64_t get_authenticator_id(void) {
    return 0;
}

static void reset_timeout(char* token, int len) {

}

static void add_lockout_reset_callback(struct lockout_reset_callback* callback) {

}

static int init(void) {

    return 0;
}

static int deinit(void) {

    return 0;
}

/* ============================== Public API end ============================ */

static struct fingerprint_manager this = {
        .init = init,
        .deinit = deinit,
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
