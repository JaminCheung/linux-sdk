#include <string.h>
#include <limits.h>
#include <stdio.h>

#include <types.h>
#include <utils/log.h>
#include <utils/common.h>
#include <utils/file_ops.h>
#include <utils/signal_handler.h>

#include "../../ma_fingerprint.h"

#define LOG_TAG "test_microarray_fp"

#define HAL_LIBRARY_PATH1 "/lib/libfprint-mips.so"
#define HAL_LIBRARY_PATH2 "/usr/lib/libfprint-mips.so"

enum {
    ENROLL_TEST,
    AUTH_TEST,
    DELETE_TEST,
    TEST_MAX,
};

enum {
    STATE_IDLE,
    STATE_BUSY
};

static struct signal_handler* signal_handler;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static int work_state = STATE_IDLE;
static int enroll_steps;

static const char* test2string(int item) {
    switch (item) {
    case ENROLL_TEST:
        return "enroll test";
    case AUTH_TEST:
        return "authenticate test";
    case DELETE_TEST:
        return "delete test";
    default:
        return "unknown test";
    }
}

static void print_tips(void) {
    fprintf(stderr, "==================== Microarray FP ===================\n");
    fprintf(stderr, "  %d.Enroll\n", ENROLL_TEST);
    fprintf(stderr, "  %d.Authenticate\n", AUTH_TEST);
    fprintf(stderr, "  %d.Delete\n", DELETE_TEST);
    fprintf(stderr, "======================================================\n");
}

static void print_delete_id_tips(void) {
    fprintf(stderr, "==============================================\n");
    fprintf(stderr, "Please enter finger id.\n");
    fprintf(stderr, "==============================================\n");
}

static void set_state_idle(void) {
    pthread_mutex_lock(&lock);

    work_state = STATE_IDLE;
    pthread_cond_signal(&cond);

    pthread_mutex_unlock(&lock);
}

static void set_state_busy(void) {
    pthread_mutex_lock(&lock);

    work_state = STATE_BUSY;
    pthread_cond_signal(&cond);

    pthread_mutex_unlock(&lock);
}

static void wait_state_idle(void) {
    pthread_mutex_lock(&lock);

    while (work_state == STATE_BUSY) {
        pthread_cond_wait(&cond, &lock);
        msleep(600);
    }

    pthread_mutex_unlock(&lock);
}

static void ma_fp_callback(const fingerprint_msg_t *msg) {
    switch (msg->type) {
    case FINGERPRINT_ERROR:
        LOGI("========> FINGERPRINT_ERROR: error=%d\n", msg->data.error);
        set_state_idle();
        break;

    case FINGERPRINT_ACQUIRED:
        LOGI("========> FINGERPRINT_ACQUIRED: acquired_info=%d\n",
                msg->data.acquired.acquired_info);
        break;

    case FINGERPRINT_AUTHENTICATED:
        LOGI("========> FINGERPRINT_AUTHENTICATED: fid=%d\n",
                msg->data.authenticated.finger.fid);
        if (msg->data.authenticated.finger.fid == 0)
            LOGE("==========> FINGERPRINT AUTH FAILED!!!\n");
        set_state_idle();
        break;

    case FINGERPRINT_TEMPLATE_ENROLLING:
        LOGI("========> FINGERPRINT_TEMPLATE_ENROLLING: fid=%d, rem=%d\n",
                msg->data.enroll.finger.fid,
                msg->data.enroll.samples_remaining);

        if (enroll_steps == -1)
            enroll_steps = msg->data.enroll.samples_remaining;

        int progress = (enroll_steps + 1 - msg->data.enroll.samples_remaining);

        LOGI("=======> ENROLL PROGRESS %d%%\n", 100 * progress / (enroll_steps + 1));

        if (msg->data.enroll.samples_remaining == 0)
            set_state_idle();
        break;

    case FINGERPRINT_TEMPLATE_REMOVED:
        LOGI("========> FINGERPRINT_TEMPLATE_REMOVED: fid=%d\n",
                msg->data.removed.finger.fid);
        set_state_idle();
        break;

    default:
        LOGE("Invalid msg type: %d", msg->type);
        break;
    }
}

static void handle_signal(int signal) {
    int error = 0;

    error = ma_fingerprint_close();
    if (error < 0)
        LOGE("Failed to close microarray library\n");

    exit(1);
}

#if 0
int init_fp_lib(char *path, uint32_t len) {
    snprintf(path, len, "%s", HAL_LIBRARY_PATH1);

    if (file_exist(path) != 0) {
        LOGW("Failed to found lib at: %s, try %s\n", HAL_LIBRARY_PATH1, HAL_LIBRARY_PATH2);

        snprintf(path, len, "%s", HAL_LIBRARY_PATH2);

        if (file_exist(path) != 0) {
            LOGE("Failed to found any microarray library\n");
            return -1;
        }
    }

    return 0;
}
#endif

static int do_enroll(void) {
    int error = 0;

    enroll_steps = -1;

    error = ma_fingerprint_enroll();
    if (error < 0) {
        LOGE("Failedl to enroll fingerprint\n");
        return -1;
    }

    return 0;
}

static int do_auth(void) {
    int error = 0;

    error = ma_fingerprint_authenticate();
    if (error < 0) {
        LOGE("Failed to auth fingerprint\n");
        return -1;
    }

    return 0;
}

static int do_delete(void) {
    int error = 0;
    char id_buf[128] = {0};

restart:
    print_delete_id_tips();

    if (fgets(id_buf, sizeof(id_buf), stdin) == NULL)
        goto restart;

    uint32_t id = strtol(id_buf, NULL, 0);
    error = ma_fingerprint_remove(id);
    if (error < 0)
        LOGE("Failed to delete finger by id %d\n", id);

    return error;
}

static void do_work(void) {
    int error = 0;
    char sel_buf[64] = {0};

    setbuf(stdin, NULL);

    for (;;) {
restart:
        wait_state_idle();

        ma_fingerprint_cancel();

        print_tips();

        if (fgets(sel_buf, sizeof(sel_buf), stdin) == NULL)
            goto restart;

        if (strlen(sel_buf) != 2)
            goto restart;

        int action = strtol(sel_buf, NULL, 0);
        if (action >= TEST_MAX || action < 0)
            goto restart;

        LOGI("Going to %s\n", test2string(action));

        set_state_busy();

        switch(action) {
        case ENROLL_TEST:
            error = do_enroll();
            break;

        case AUTH_TEST:
            error = do_auth();
            break;

        case DELETE_TEST:
            error = do_delete();
            break;

        default:
            break;
        }
    }
}

int main(int argc, char *argv[]) {
    char path[PATH_MAX];

    int error = 0;

    signal_handler = _new(struct signal_handler, signal_handler);

    signal_handler->set_signal_handler(signal_handler, SIGINT, handle_signal);
    signal_handler->set_signal_handler(signal_handler, SIGQUIT, handle_signal);
    signal_handler->set_signal_handler(signal_handler, SIGTERM, handle_signal);

#if 0
    error = init_fp_lib(path, sizeof(path));
    if (error < 0)
        return -1;
#endif

    error = ma_fingerprint_set_notify_callback(ma_fp_callback);
    if (error < 0) {
        LOGE("Failed to set callback\n");
        return -1;
    }

    error = ma_fingerprint_open(get_user_system_dir(getuid()));
    if (error < 0) {
        LOGE("Failed to open microarray lib\n");
        return -1;
    }

    do_work();

    while (1)
        sleep(1000);

    return 0;
}
