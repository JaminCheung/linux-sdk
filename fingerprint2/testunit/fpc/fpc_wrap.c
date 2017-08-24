#include <poll.h>
#include <string.h>

#include <utils/log.h>
#include <utils/assert.h>
#include <utils/common.h>
#include <utils/runnable/default_runnable.h>
#include "fpc_wrap.h"

#define LOG_TAG "fpc_wrap"

static struct default_runnable* thread_runner;
static notify_callback callback;
static customer_config_t config;
static int local_pipe[2];

enum {
    MSG_QUIT = 0,
    MSG_ENROLL,
    MSG_AUTHENTICATE,
    MSG_DELETE,
};

static const char* msg2str(uint8_t msg) {
    switch(msg) {
    case MSG_QUIT:
        return "quit";

    case MSG_ENROLL:
        return "enroll";

    case MSG_AUTHENTICATE:
        return "authenticate";

    case MSG_DELETE:
        return "delete";

    default:
        return "unknown";
    }
}

static int send_msg(uint8_t msg) {
    int error = 0;

    error = write(local_pipe[1], &msg, 1);
    if (error != 0) {
        LOGE("Failed to send \'%s\'\n", msg2str(msg));
        return -1;
    }

    return 0;
}

static int handle_msg(uint8_t msg) {
    switch(msg) {
    case MSG_QUIT:
        return 1;

    case MSG_ENROLL:
        break;

    case MSG_AUTHENTICATE:
        break;

    case MSG_DELETE:
        break;

    default:
        break;
    }

    return 0;
}

static void thread_loop(struct pthread_wrapper* thread, void* param) {
    int count;
    int error;
    uint8_t msg;

    struct pollfd fds[1];

    fds[0].fd = local_pipe[0];
    fds[0].events = POLLIN;
    fds[0].revents = 0;

    for (;;) {

        do {
            count = poll(fds, 2, -1);
        } while (count < 0 && errno == EINTR);

        if (fds[0].revents & POLLIN) {
            fds[0].revents = 0;
            error = read(fds[0].fd, &msg, 1);
            if (!error) {
                LOGE("Unable to read pipe: %s\n", strerror(errno));
                continue;
            }

            if (handle_msg(msg) > 0) {
                LOGI("main thread call me break out\n");
                break;
            }
        }
    }
}

int fpc_fingerprint_init(notify_callback notify, void *param_config) {
    assert_die_if(notify == NULL, "notify is NULL\n");
    assert_die_if(notify == NULL, "param_config is NULL\n");

    int error = 0;

    callback = notify;
    memcpy(&config, param_config, sizeof(customer_config_t));

    error = pipe(local_pipe);
    if (error) {
        LOGE("Unable to open pipe: %s\n", strerror(errno));
        return -1;
    }

    thread_runner = _new(struct default_runnable, default_runnable);
    thread_runner->runnable.run = thread_loop;
    thread_runner->start(thread_runner, NULL);

    return -1;
}

int fpc_fingerprint_destroy(void) {
    if (send_msg(MSG_QUIT)) {
        LOGE("Failed to quit thread runner\n");
        return -1;
    }

    thread_runner->wait(thread_runner);
    _delete(thread_runner);

    if (local_pipe[0] > 0)
        close(local_pipe[0]);

    if (local_pipe[1] > 0)
        close(local_pipe[1]);

    return -1;
}

int fpc_fingerprint_reset(void) {
    return -1;
}

int fpc_fingerprint_enroll(void) {
    return -1;
}

int fpc_fingerprint_authenticate(void) {
    return -1;
}

int fpc_fingerprint_delete(int fingerprint_id, int type) {
    return -1;
}

int fpc_fingerprint_cancel(void) {
    return -1;
}

int fpc_fingerprint_get_template_info(int template_info[]) {
    return -1;
}
