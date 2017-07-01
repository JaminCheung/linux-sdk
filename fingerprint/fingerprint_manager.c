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

#include <unistd.h>
#include <sys/types.h>

#include <utils/log.h>
#include <utils/assert.h>
#include <utils/common.h>
#include <utils/runnable/default_runnable.h>
#include <fingerprint/fingerprint_manager.h>
#include <fingerprint/fingerprint_errorcode.h>
#include "fingerprint_utils.h"
#include "hardware/fingerprint_device_proxy.h"
#include "hardware/fingerprint_device_callback.h"
#include "fingerprint_client_sender.h"
#include "client_monitor.h"
#include "enroll_client.h"
#include "authentication_client.h"
#include "removal_client.h"
#include "enumerate_client.h"

#define LOG_TAG "fingerprint_manager"

typedef enum client_type {
    ENROLL_CLIENT,
    AUTHENTICATION_CLIENT,
    REMOVAL_CLIENT,
    ENUMERATE_CLIENT,
} client_type_t;

static const int enroll_limit = 10;

static const int user_id;

static struct enroll_param {
    int user_id;
    int flag;
    char* token;
    int token_len;
} enroll_param;

static struct authenticate_param {
    int op_id;
    int user_id;
    int group_id;
    int flags;
} authenticate_param;

static struct remove_param {
    int user_id;
    struct fingerprint* fp;
} remove_param;

static struct rename_param {
    int finger_id;
    int group_id;
    const char* name;
} rename_param;

static struct enroll_result_param {
    int64_t device_id;
    int finger_id;
    int group_id;
    int remaining;
} enroll_result_param;

static struct acquired_result_param {
    int64_t device_id;
    int acquired_info;
} acquired_result_param;

static struct authenticated_result_param {
    int64_t device_id;
    int finger_id;
    int group_id;
} authenticated_result_param;

static struct error_result_param {
    int64_t device_id;
    int error;
} error_result_param;

static struct removed_result_param {
    int64_t device_id;
    int finger_id;
    int group_id;
} removed_result_param;

static struct enumerated_result_param {
    int64_t device_id;
    const int* finger_id;
    int finger_size;
    const int* group_id;
    int group_size;
} enumerated_result_param;

struct client_monitor* current_client;
struct enroll_client* enroll_client;
struct authentication_client* authentication_client;
struct removal_client* removal_client;

static struct fingerprint_client_sender* client_sender;

static struct authentication_callback* authentication_callback;
static struct enrollment_callback* enrollment_callback;
static struct removal_callback* removal_callback;

static struct fingerprint* removal_fingerprint;

static struct fingerprint_device_proxy* device_proxy;
static struct fingerprint_utils* fingerprint_utils;
static int64_t hardware_device_id;


static struct default_runnable* enroll_runner;
static struct default_runnable* authentication_runner;
static struct default_runnable* active_group_runner;
static struct default_runnable* remove_runner;
static struct default_runnable* rename_runner;

static struct default_runnable* enroll_result_runner;
static struct default_runnable* authenticated_result_runner;
static struct default_runnable* remove_result_runner;
static struct default_runnable* error_result_runner;
static struct default_runnable* acquired_result_runner;
static struct default_runnable* enumerated_result_runner;

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

static void remove_client(struct client_monitor* client) {

}

static void handle_enroll_result(int64_t device_id, int finger_id, int group_id,
        int remaining) {
    struct client_monitor* client = current_client;
    if (client != NULL) {
        if (client->on_enroll_result(client, finger_id, group_id, remaining)) {
            remove_client(client);
        }
    }
}

static void handle_acquired(int64_t device_id, int acquired_info) {
    struct client_monitor* client = current_client;

    if (client != NULL) {
        if (client->on_acquired(client, acquired_info)) {
            remove_client(client);
        }
    }
}

static void handle_authenticated(int64_t device_id, int finger_id,
        int group_id) {
    struct client_monitor* client = current_client;

    if (client != NULL) {
        if (client->on_authenticated(client, finger_id, group_id)) {
            remove_client(client);
        }
    }
}

static void handle_removed(int64_t device_id, int finger_id, int group_id) {
    struct client_monitor* client = current_client;

    if (client != NULL) {
        if (client->on_removed(client, finger_id, group_id)) {
            remove_client(client);
        }
    }
}

static void handle_enumerate(int64_t device_id, const int* finger_id, int finger_size,
        const int* group_ids, int group_size) {
    if (finger_size != group_size) {
        LOGE("Finger id and group id length different\n");
        return;
    }
}

static void handle_error(int64_t device_id, int error) {
    struct client_monitor* client = current_client;

    if (client != NULL) {
        if (client->on_error(client, error))
            remove_client(client);
    }
}

static void enroll_result_thread(struct pthread_wrapper* thread, void* param) {
    struct enroll_result_param* args = (struct enroll_result_param*)param;

    handle_enroll_result(args->device_id, args->finger_id, args->group_id,
            args->remaining);
}

static void acquired_result_thread(struct pthread_wrapper* thread, void* param) {
    struct acquired_result_param *args = (struct acquired_result_param*)param;

    handle_acquired(args->device_id, args->acquired_info);
}

static void error_result_thread(struct pthread_wrapper* thread, void* param) {
    struct error_result_param* args = (struct error_result_param*)param;

    handle_error(args->device_id, args->error);
}

static void removed_result_thread(struct pthread_wrapper* thread, void* param) {
    struct removed_result_param* args = (struct removed_result_param*)param;

    handle_removed(args->device_id, args->finger_id, args->group_id);
}

static void enumerated_result_thread(struct pthread_wrapper* thread, void* param) {
    struct enumerated_result_param* args =
            (struct enumerated_result_param*)param;

    handle_enumerate(args->device_id, args->finger_id, args->finger_size,
            args->group_id, args->group_size);
}

static void authenticated_result_thread(struct pthread_wrapper* threaed, void* param) {
    struct authenticated_result_param* args =
            (struct authenticated_result_param*)param;

    handle_authenticated(args->device_id, args->finger_id, args->group_id);
}

static void on_enroll_result(int64_t device_id, int finger_id, int group_id,
        int samples_remaining) {
    enroll_result_param.device_id = device_id;
    enroll_result_param.finger_id = finger_id;
    enroll_result_param.group_id = group_id;
    enroll_result_param.remaining = samples_remaining;

    enroll_runner->start(enroll_runner, (void*)&enroll_result_param);
}

static void on_acquired(int64_t device_id, int acquired_info) {
    acquired_result_param.device_id = device_id;
    acquired_result_param.acquired_info = acquired_info;

    acquired_result_runner->start(acquired_result_runner,
            (void*)&acquired_result_param);
}

static void on_authenticated(int64_t device_id, int finger_id, int group_id) {
    authenticated_result_param.device_id = device_id;
    authenticated_result_param.finger_id = finger_id;
    authenticated_result_param.group_id = group_id;

    authenticated_result_runner->start(authenticated_result_runner,
            (void*)&authenticated_result_param);
}

static void on_error(int64_t device_id, int error) {
    error_result_param.device_id = device_id;
    error_result_param.error = error;

    error_result_runner->start(error_result_runner, (void*)&error_result_param);
}

static void on_removed(int64_t device_id, int finger_id, int group_id) {
    removed_result_param.device_id = device_id;
    removed_result_param.finger_id = finger_id;
    removed_result_param.group_id = group_id;

    remove_result_runner->start(remove_result_runner,
            (void*)&removed_result_param);
}

static void on_enumerate(int64_t device_id, const int* finger_id, int finger_size,
        const int* group_id, int group_size) {
    enumerated_result_param.device_id = device_id;
    enumerated_result_param.finger_id = finger_id;
    enumerated_result_param.finger_size = finger_size;
    enumerated_result_param.group_id = group_id;
    enumerated_result_param.group_size = group_size;

    enumerated_result_runner->start(enumerated_result_runner,
            (void*)&enumerated_result_param);
}

static struct fingerprint_device_callback device_callback = {
        .on_enroll_result = on_enroll_result,
        .on_acquired = on_acquired,
        .on_authenticated = on_authenticated,
        .on_error = on_error,
        .on_removed = on_removed,
        .on_enumerate = on_enumerate,
};

static void update_active_group(int group_id, const char* progress_name) {

}

static void start_client(struct client_monitor* new_client,
        int initialed_by_client) {
    int error = 0;

    if (current_client != NULL) {
        error = current_client->stop(current_client, initialed_by_client);
        if (error) {
            LOGE("Failed to stop client\n");
            return;
        }
    }

    if (new_client != NULL) {
        LOGD("Start client\n");
        current_client = new_client;
        current_client->start(current_client);
    }
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

static int can_use_fingerprint(void) {
    return 1;
}

static uint64_t start_pre_enroll(void) {
    return device_proxy->pre_enroll();
}

static int start_post_enroll(void) {
    return device_proxy->post_enroll();
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



static void start_enrollment(char* token, int token_len, int user_id, int flags) {
    update_active_group(user_id, NULL);

    int group_id = user_id;

    if (enroll_client == NULL) {
        enroll_client = calloc(1, sizeof(struct enroll_client));
        enroll_client->construct = construct_enroll_client;
        enroll_client->destruct = destruct_enroll_client;
        enroll_client->construct(enroll_client, hardware_device_id,
                client_sender, user_id, group_id, token, token_len);
    }

    start_client(enroll_client->base, 1);
}

static void enroll_thread(struct pthread_wrapper* thread, void* param) {
    struct enroll_param* args = (struct enroll_param*)param;

    start_enrollment(args->token, args->token_len, args->user_id, args->flag);
}

static void start_authentication(int64_t op_id, int user_id, int group_id,
        int flags) {
    update_active_group(group_id, NULL);

    if (authentication_client == NULL) {
        authentication_client = calloc(1, sizeof(struct authentication_client));
        authentication_client->construct = construct_authentication_client;
        authentication_client->destruct = destruct_authentication_client;
        authentication_client->construct(authentication_client,
                hardware_device_id, client_sender, user_id, group_id, op_id);
    }

    start_client(authentication_client->base, 1);
}

static void authenticate_thread(struct pthread_wrapper* thread, void* param) {
    struct authenticate_param *args = (struct authenticate_param*) param;

    start_authentication(args->op_id, args->user_id, args->group_id,
            args->flags);
}

static void active_group_thread(struct pthread_wrapper* thread, void* param) {
    int user_id = (int)param;

    update_active_group(user_id, NULL);
}

void start_remove(int finger_id, int group_id, int user_id) {
    if (removal_client == NULL) {
        removal_client = calloc(1, sizeof(struct removal_client));
        removal_client->construct = construct_removal_client;
        removal_client->destruct = destruct_removal_client;
        removal_client->construct(removal_client, hardware_device_id,
                client_sender, finger_id, group_id, user_id);
    }

    start_client(removal_client->base, 1);
}

static void remove_thread(struct pthread_wrapper* thread, void* param) {
    struct remove_param* args = (struct remove_param*)param;

    struct fingerprint* fp = args->fp;

    start_remove(fp->get_finger_id(fp), fp->get_group_id(fp), user_id);
}

static void rename_thread(struct pthread_wrapper* thread, void* param) {
    struct rename_param* args = (struct rename_param*)param;

    fingerprint_utils->rename_fingerprint_for_user(args->finger_id,
            args->group_id, args->name);
}

/* =========================== Client Interface ============================= */

static void notify_enroll_result(struct fingerprint* fp, int remaining) {
    if (enrollment_callback != NULL)
        enrollment_callback->on_enrollment_progress(remaining);
}

static void send_enroll_result(int64_t device_id, int finger_id, int group_id,
        int remaining) {
    struct fingerprint* fp = calloc(1, sizeof(struct fingerprint));
    fp->construct = construct_fingerprint;
    fp->destruct = destruct_fingerprint;
    fp->construct(fp, "NULL", group_id, finger_id, device_id);

    notify_enroll_result(fp, remaining);

    fp->destruct(fp);
    free(fp);
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

static void send_acquired_result(int64_t device_id, int acquire_info) {
    notify_acquired_result(device_id, acquire_info);
}

static void notify_authentication_successed(struct fingerprint* fp, int user_id) {
    if (authentication_callback != NULL) {
        struct authentication_result* result =
                calloc(1, sizeof(struct authentication_result));
        result->construct = construct_authentication_result;
        result->destruct = destruct_authentication_result;
        result->construct(result, fp, user_id);

        authentication_callback->on_authentication_successed(result);

        result->destruct(result);
        free(result);
    }
}

static void send_authentication_successed(int64_t device_id,
        struct fingerprint* fp, int user_id) {
    notify_authentication_successed(fp, user_id);
}

static void notify_authentication_failed(void) {
    if (authentication_callback != NULL)
        authentication_callback->on_authentication_failed();
}

static void send_authentication_failed(int64_t device_id) {
    notify_authentication_failed();
}

static void notify_removed_result(int64_t device_id, int finger_id, int group_id) {
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

static void send_removed_result(int64_t device_id, int finger_id, int group_id) {
    notify_removed_result(device_id, finger_id, group_id);
}

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

static void send_error_result(int64_t device_id, int error) {
    notify_error_result(device_id, error);
}

/* ========================= Client Interface end =========================== */




/* ================================ Public API ============================== */

static int is_hardware_detected(void) {
    //TODO: to service
    if (!can_use_fingerprint())
        return 0;

    return hardware_device_id != 0;
}

static struct fingerprint_list* get_enrolled_fingerprints(int user_id) {
    return fingerprint_utils->get_fingerprints_for_user(user_id);
}

static int has_enrolled_fingerprints(int user_id) {
    //TODO: to service

    if (!can_use_fingerprint())
        return 0;

    struct fingerprint_list* list = fingerprint_utils->get_fingerprints_for_user(user_id);

    return list->size(list) > 0;
}

static void authenticate(struct authentication_callback *callback, int flag,
        int user_id) {
    assert_die_if (callback == NULL, "Callback is NULL\n");

    if (!can_use_fingerprint())
        return;


    authentication_callback = callback;

    int session_id = 0;
    //TODO: to service

    authenticate_param.flags = flag;
    authenticate_param.user_id = user_id;
    authenticate_param.group_id = user_id;
    authenticate_param.op_id = 0;

    authentication_runner->start(authentication_runner, (void*)&authenticate_param);
}

static void enroll(char* token, int token_len, int flags, int user_id,
        struct enrollment_callback *callback) {
    assert_die_if(callback == NULL, "Callback is NULL\n");

    enrollment_callback = callback;
    //TODO: to service

    struct fingerprint_list* list = get_enrolled_fingerprints(user_id);

    int enrolled = list->size(list);
    if (enrolled >= enroll_limit) {
        LOGW("Too many fingerprints registerd\n");
        return;
    }

    enroll_param.flag = flags;
    enroll_param.user_id = user_id;
    enroll_param.token = token;
    enroll_param.token_len = token_len;

    enroll_runner->start(enroll_runner, (void*)&enroll_param);
}

static int64_t pre_enroll(void) {
    return start_pre_enroll();
}

static int post_enroll(void) {
    return start_post_enroll();
}

static void set_active_user(int user_id) {
    active_group_runner->start(active_group_runner, (void*)&user_id);
}

static void remove_fingerprint(struct fingerprint *fp, int user_id,
        struct removal_callback* callback) {
    removal_callback = callback;
    removal_fingerprint = fp;
    //TODO: to service


    remove_param.fp = fp;
    remove_param.user_id = user_id;

    remove_runner->start(remove_runner, (void*)&remove_param);
}

static void rename_fingerprint(int fp_id, int user_id, const char* new_name) {
    rename_param.finger_id = fp_id;
    rename_param.group_id = getegid();
    rename_param.name = new_name;

    rename_runner->start(rename_runner, (void*)&rename_param);
}

static int64_t get_authenticator_id(void) {
    //TODO: to service
    return 0;
}

static void reset_timeout(char* token, int token_len) {
    //TODO: to service
}

static void add_lockout_reset_callback(struct lockout_reset_callback* callback) {
    LOGI("Please implement me.\n");
}

static int init(void) {
    device_proxy = get_fingerprint_device_proxy();

    device_proxy->init(&device_callback);
    hardware_device_id = device_proxy->open_hal();
    if (hardware_device_id != 0) {
        update_active_group(0, NULL);
    } else {
        LOGE("Failed to open fingerprint device\n");
        return -1;
    }

    fingerprint_utils = get_fingerprint_utils();

    client_sender = calloc(1, sizeof(struct fingerprint_client_sender));
    client_sender->on_enroll_result = send_enroll_result;
    client_sender->on_acquired = send_acquired_result;
    client_sender->on_authentication_successed = send_authentication_successed;
    client_sender->on_authentication_failed = send_authentication_failed;
    client_sender->on_error = send_error_result;
    client_sender->on_removed = send_removed_result;

    enroll_runner = _new(struct default_runnable, default_runnable);
    enroll_runner->runnable.run = enroll_thread;
    authentication_runner = _new(struct default_runnable, default_runnable);
    authentication_runner->runnable.run = authenticate_thread;
    active_group_runner = _new(struct default_runnable, default_runnable);
    active_group_runner->runnable.run = active_group_thread;
    remove_runner = _new(struct default_runnable, default_runnable);
    remove_runner->runnable.run = remove_thread;
    rename_runner = _new(struct default_runnable, default_runnable);
    rename_runner->runnable.run = rename_thread;

    enroll_result_runner = _new(struct default_runnable, default_runnable);
    enroll_result_runner->runnable.run = enroll_result_thread;
    authenticated_result_runner = _new(struct default_runnable, default_runnable);
    authenticated_result_runner->runnable.run = authenticated_result_thread;
    remove_result_runner = _new(struct default_runnable, default_runnable);
    remove_result_runner->runnable.run = removed_result_thread;
    error_result_runner = _new(struct default_runnable, default_runnable);
    error_result_runner->runnable.run = error_result_thread;
    acquired_result_runner = _new(struct default_runnable, default_runnable);
    acquired_result_runner->runnable.run = acquired_result_thread;
    enumerated_result_runner = _new(struct default_runnable, default_runnable);
    enumerated_result_runner->runnable.run = enumerated_result_thread;

    return 0;
}

static int deinit(void) {
    free(client_sender);

    _delete(enroll_runner);
    _delete(authentication_runner);
    _delete(active_group_runner);
    _delete(remove_runner);
    _delete(rename_runner);

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
