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
#include <utils/assert.h>
#include <utils/common.h>
#include <fingerprint/fingerprint_manager.h>
#include <fingerprint/fingerprint_errorcode.h>
#include "fingerprint_utils.h"
#include "fingerprint_device_proxy.h"
#include "fingerprint_device_callback.h"

#define LOG_TAG "fingerprint_manager"

static const int user_id;

static struct authentication_callback* authentication_callback;
static struct enrollment_callback* enrollment_callback;
static struct removal_callback* removal_callback;

static struct fingerprint* removal_fingerprint;

static struct fingerprint_device_proxy* device_proxy;
static struct fingerprint_utils* fingerprint_utils;
static int64_t hardware_device_id;

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

static void on_enroll_result(int64_t device_id, int finger_id, int group_id,
        int samples_remaining) {

}

static void on_acquired(int64_t device_id, int acquired_info) {

}

static void on_authenticated(int64_t device_id, int finger_id, int group_id) {

}

static void on_error(int64_t device_id, int error) {

}

static void on_removed(int64_t device_id, int finger_id, int group_id) {

}

static void on_enumerate(int64_t device_id, const int* finger_id,
        const int* group_id, int size) {

}

static struct fingerprint_device_callback device_callback = {
        .on_enroll_result = on_enroll_result,
        .on_acquired = on_acquired,
        .on_authenticated = on_authenticated,
        .on_error = on_error,
        .on_removed = on_removed,
        .on_enumerate = on_enumerate,
};

static void notify_error_result(int64_t device_id, int error_code) {
    if (enrollment_callback != NULL)
        enrollment_callback->on_enrollment_error(error_code,
                get_error_string(error_code));
    if (authentication_callback != NULL)
        authentication_callback->on_authentication_error(error_code,
                get_error_string(error_code));
    if (removal_callback != NULL)
        removal_callback->on_removal_error(removal_fingerprint, error_code,
                get_error_string(error_code));
}

static void noity_removed_result(int64_t device_id, int finger_id, int group_id) {
    if (removal_callback != NULL) {
        int req_finger_id = removal_fingerprint->get_finger_id(removal_fingerprint);
        int req_group_id = removal_fingerprint->get_group_id(removal_fingerprint);

        if (req_finger_id != 0 && finger_id != 0 && finger_id != req_finger_id) {
            LOGW("Finger id did't match: %d != %d\n", req_finger_id, finger_id);
            return;
        }

        if (group_id != req_group_id) {
            LOGW("Group id did't match: %d != %d\n", req_group_id, group_id);
            return;
        }

        struct fingerprint* fp = calloc(1, sizeof(struct fingerprint));
        fp->construct = construct_fingerprint;
        fp->destruct = destruct_fingerprint;
        fp->construct(fp, "NULL", group_id, finger_id, device_id);

        removal_callback->on_removal_successed(fp);

        fp->destruct(fp);
        free(fp);
    }
}

static void notify_enroll_result(struct fingerprint* fp, int remaining) {
    if (enrollment_callback != NULL)
        enrollment_callback->on_enrollment_progress(remaining);
}

static void notify_authenticated_success(struct fingerprint* fp) {
    if (authentication_callback != NULL) {
        struct authentication_result* result =
                calloc(1, sizeof(struct authentication_result));
        result->construct = construct_authentication_result;
        result->destruct = destruct_authentication_result;
        result->construct(result, fp, 0);

        authentication_callback->on_authentication_successed(result);

        result->destruct(result);
        free(result);
    }
}

static void notify_authenticated_failed(void) {
    if (authentication_callback != NULL)
        authentication_callback->on_authentication_failed();
}

static void notify_acquired_result(int64_t device_id, int acquired_info) {
    if (authentication_callback != NULL)
        authentication_callback->on_authentication_acquired(acquired_info);

    const char* str = get_acquired_string(acquired_info);
    if (str == NULL)
        return;

    if (enrollment_callback != NULL)
        enrollment_callback->on_enrollment_help(acquired_info, str);
    else if (authentication_callback != NULL)
        authentication_callback->on_authentication_help(acquired_info, str);
}

static void stop_authentication(int initialed_by_client) {

}

static void stop_enrollment(int initialed_by_client) {
    int error = 0;

    if (initialed_by_client) {
        error = device_proxy->stop_enrollment();
        LOGW_IF(error, "stop_enrollment failed, error: %d\n", error);

    }
}

static void stop_pending_operations(int initialted_by_client) {
    stop_enrollment(initialted_by_client);
    stop_authentication(initialted_by_client);
}

static void handle_error(int64_t device_id, int error) {

}

static void update_active_group() {

}

static int can_use_fingerprint(void) {
    return 1;
}

static uint64_t start_pre_enroll(void) {
    return device_proxy->pre_enroll();
}

static int start_post_enroll(void) {
    return device_proxy->post_enroll();
}

static void start_remove(int finger_id, int user_id) {
    int error = 0;

    stop_pending_operations(1);

    error = device_proxy->remove_fingerprint(finger_id, user_id);
    if (error) {
        LOGW("startRemove with id = %d failed, result=%d\n",finger_id, error);
        handle_error(hardware_device_id, FINGERPRINT_ERROR_HW_UNAVAILABLE);
    }
}

static int get_current_id(void) {
    return 0;
}

static void cacel_enrollment(void) {
    //TODO: to service
}

static void cancel_authentication(void) {
    //TODO: to service
}

/* ================================ Public API ============================== */

static int is_hardware_detected(void) {
    //TODO: to service
    if (!can_use_fingerprint())
        return 0;

    return hardware_device_id != 0;
}

static int has_enrolled_fingerprints(int user_id) {
    //TODO: to service

    if (!can_use_fingerprint())
        return 0;

    struct fingerprint_list* list = fingerprint_utils->get_fingerprints_for_user(user_id);

    return list->size(list);
}

static void authenticate(struct authentication_callback *callback, int flag,
        int user_id) {
    assert_die_if (callback == NULL, "Callback is NULL\n");

    if (!can_use_fingerprint())
        return;


    authentication_callback = callback;

    int session_id = 0;
    //TODO: to service
}

static void enroll(char* token, int token_len, int flags, int user_id,
        struct enrollment_callback *callback) {
    assert_die_if(callback == NULL, "Callback is NULL\n");

    enrollment_callback = callback;
    //TODO: to service
}

static int64_t pre_enroll(void) {
    //TODO: to service
    return start_pre_enroll();
}

static int post_enroll(void) {
    //TODO: to service
    return start_post_enroll();
}

static void set_active_user(int user_id) {
    //TODO: to service
}

static void remove_fingerprint(struct fingerprint *fp, int user_id,
        struct removal_callback* callback) {
    removal_callback = callback;
    removal_fingerprint = fp;
    //TODO: to service
}

static void rename_fingerprint(int fp_id, int user_id, const char* new_name) {

}

static struct fingerprint_list* get_enrolled_fingerprints(int user_id) {
    //TODO: to service

    return NULL;
}

static int64_t get_authenticator_id(void) {
    //TODO: to service
    return 0;
}

static void reset_timeout(char* token, int token_len) {
    //TODO: to service
}

static void add_lockout_reset_callback(struct lockout_reset_callback* callback) {
    LOGI("Please implemnt me.\n");
}

static int init(void) {
    device_proxy = get_fingerprint_device_proxy();

    device_proxy->init(&device_callback);
    hardware_device_id = device_proxy->open_hal();
    if (hardware_device_id != 0) {
        update_active_group();
    } else {
        LOGE("Failed to open fingerprint device\n");
        return -1;
    }

    fingerprint_utils = get_fingerprint_utils();

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
        .set_active_user = set_active_user,
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
