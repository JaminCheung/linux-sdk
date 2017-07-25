#include <string.h>

#include <types.h>
#include <utils/log.h>
#include <utils/common.h>
#include <utils/file_ops.h>
#include <utils/signal_handler.h>
#include <power/power_manager.h>

#include "../../goodix_interface.h"

#define LOG_TAG "test_goodix_fp"

typedef enum {
    DELETE_BY_ID,
    DELETE_ALL,
} delete_type_t;

typedef enum {
    FP_MSG_FINGER_DOWN          = 0,
    FP_MSG_FINGER_UP            = 1,
    FP_MSG_NO_LIVE              = 2,
    FP_MSG_PROCESS_SUCCESS      = 3,
    FP_MSG_PROCESS_FAILED       = 4,
    FP_MSG_ENROLL_SUCCESS       = 5,
    FP_MSG_ENROLL_FAILED        = 6,
    FP_MSG_MATCH_SUCESS         = 7,
    FP_MSG_MATCH_FAILED         = 8,
    FP_MSG_CANCEL               = 9,
    FP_MSG_TIMEOUT              = 10,
    FP_MSG_MAX
}fp_msg_t;

enum {
    ENROLL_TEST,
    AUTH_TEST,
    DELETE_TEST,
    SUSPEND_TEST,
    TEST_MAX,
};

enum {
    STATE_IDLE,
    STATE_BUSY
};

static struct power_manager* power_manager;
static struct signal_handler* signal_handler;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static int work_state = STATE_IDLE;

static const char* test2string(int item) {
    switch (item) {
    case ENROLL_TEST:
        return "enroll test";
    case AUTH_TEST:
        return "authenticate test";
    case DELETE_TEST:
        return "delete test";
    case SUSPEND_TEST:
        return "suspend test";
    default:
        return "unknown test";
    }
}

static void print_tips(void) {
    fprintf(stderr, "====================== Goodix FP =====================\n");
    fprintf(stderr, "  %d.Enroll\n", ENROLL_TEST);
    fprintf(stderr, "  %d.Authenticate\n", AUTH_TEST);
    fprintf(stderr, "  %d.Delete\n", DELETE_TEST);
    fprintf(stderr, "  %d.Suspend\n", SUSPEND_TEST);
    fprintf(stderr, "======================================================\n");
}

static void print_delete_tips(void) {
    fprintf(stderr, "==============================================\n");
    fprintf(stderr, "Please select delete type.\n");
    fprintf(stderr, "  %d.Delete by id\n", DELETE_BY_ID);
    fprintf(stderr, "  %d.Delete all\n", DELETE_ALL);
    fprintf(stderr, "==============================================\n");
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
        msleep(500);
    }

    pthread_mutex_unlock(&lock);
}

static void goodix_fp_callback(int msg, int percent, int finger_id) {
    switch(msg) {
    case FP_MSG_FINGER_DOWN:
        LOGI("========> FP_MSG_FINGER_DOWN , id = %d\n", finger_id);
        break;

    case FP_MSG_FINGER_UP:
        LOGI("========> FP_MSG_FINGER_UP , id = %d\n", finger_id);
        break;

    case FP_MSG_NO_LIVE:
        LOGI("========> FP_MSG_NO_LIVE , id = %d\n", finger_id);
        set_state_idle();
        break;

    case FP_MSG_PROCESS_SUCCESS:
        LOGI("========> FP_MSG_PROCESS_SUCCESS , proc = %d\n", percent);
        break;

    case FP_MSG_PROCESS_FAILED:
        LOGI("========> FP_MSG_PROCESS_FAILED , proc = %d\n", percent);
        break;

    case FP_MSG_ENROLL_SUCCESS:
        LOGI("========> FP_MSG_ENROLL_SUCCESS , id = %d\n", finger_id);
        set_state_idle();
        break;

    case FP_MSG_ENROLL_FAILED:
        LOGI("========> FP_MSG_ENROLL_FAILED , id = %d\n", finger_id);
        set_state_idle();
        break;

    case FP_MSG_MATCH_SUCESS:
        LOGI("========> FP_MSG_MATCH_SUCESS , id = %d\n", finger_id);
        set_state_idle();
        break;

    case FP_MSG_MATCH_FAILED:
        LOGI("========> FP_MSG_MATCH_FAILED , id = %d\n", finger_id);
        set_state_idle();
        break;

    case FP_MSG_CANCEL:
        LOGI("========> FP_MSG_CANCEL\n");
        set_state_idle();
        break;

    case FP_MSG_TIMEOUT:
        LOGI("========> FP_MSG_TIMEOUT\n");
        break;

    default:
        break;
    }
}

static int do_enroll(void) {
    int error = 0;

    error = fingerprint_enroll();
    if (error < 0) {
        LOGE("Failedl to enroll fingerprint\n");
        return -1;
    }

    return 0;
}

static int do_auth(void) {
    int error = 0;

    error = fingerprint_authenticate();
    if (error < 0) {
        LOGE("Failed to auth fingerprint\n");
        return -1;
    }

    return 0;
}

static int do_delete(void) {
    int error = 0;
    char id_buf[128] = {0};
    char type_buf[32] = {0};

restart:
    print_delete_tips();

    if (fgets(type_buf, sizeof(type_buf), stdin) == NULL)
        goto restart;
    if (strlen(type_buf) != 2)
        goto restart;

    int type = strtol(type_buf, NULL, 0);
    if (type > DELETE_ALL || type < 0)
        goto restart;

    if (type == DELETE_BY_ID) {
restart2:
        print_delete_id_tips();

        if (fgets(id_buf, sizeof(id_buf), stdin) == NULL)
            goto restart2;
        int id = strtol(id_buf, NULL, 0);

        error = fingerprint_delete(id, type);
        if (error < 0)
            LOGE("Failed to delete finger by id %d\n", id);

    } else if (type == DELETE_ALL) {
        error = fingerprint_delete(0, type);
        if (error < 0)
            LOGE("Failed to delete all finger\n");

    } else {
        LOGE("Invalid delete type\n");
        goto restart;
    }

    return error;
}

static int do_suspend(void) {
    int error = 0;

    power_manager->sleep();

    LOGI("=====> Wakeup <=====\n");
    error = fingerprint_post_power_on();
    if(error)
        LOGE("Failed to post power on\n");

    error = fingerprint_reset();
    if (error)
        LOGE("Failed to reset fingerprint sensor\n");

    return 0;
}

static void do_work(void) {
    int error = 0;
    char sel_buf[64] = {0};

    setbuf(stdin, NULL);

    for (;;) {
restart:
        wait_state_idle();

        fingerprint_cancel();

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

        case SUSPEND_TEST:
            error = do_suspend();
            break;

        default:
            break;
        }
    }
}

static void handle_signal(int signal) {
    fingerprint_cancel();
    fingerprint_destroy();

    exit(1);
}

int main(int argc, char *argv[]) {
    int error = 0;

    power_manager = get_power_manager();
    signal_handler = _new(struct signal_handler, signal_handler);

    signal_handler->set_signal_handler(signal_handler, SIGINT, handle_signal);
    signal_handler->set_signal_handler(signal_handler, SIGQUIT, handle_signal);
    signal_handler->set_signal_handler(signal_handler, SIGTERM, handle_signal);

    error = fingerprint_init(goodix_fp_callback);
    if (error < 0) {
        LOGE("Failed to init fingerprint sensor\n");
        return -1;
    }

    do_work();

    while (1)
        sleep(1000);

    return 0;
}
