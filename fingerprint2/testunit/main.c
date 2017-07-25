#include <string.h>

#include <types.h>
#include <utils/log.h>
#include <utils/common.h>
#include <utils/file_ops.h>
#include <utils/signal_handler.h>
#include <power/power_manager.h>
#include <fingerprint2/fingerprint_manager.h>

#define LOG_TAG "test_fingerprint"

static struct fingerprint_manager* fingerprint_manager;
static struct power_manager* power_manager;
static struct signal_handler* signal_handler;

enum {
    ENROLL_TEST,
    AUTH_TEST,
    DELETE_TEST,
    SUSPEND_TEST,
    TEST_MAX,
};

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
    fprintf(stderr, "==============================================\n");
    fprintf(stderr, "  %d.Enroll\n", ENROLL_TEST);
    fprintf(stderr, "  %d.Authenticate\n", AUTH_TEST);
    fprintf(stderr, "  %d.Delete\n", DELETE_TEST);
    fprintf(stderr, "  %d.Suspend\n", SUSPEND_TEST);
    fprintf(stderr, "==============================================\n");
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

static void event_listener(int msg, int percent, int finger_id) {
    switch(msg) {
    case FP_MSG_FINGER_DOWN:
        LOGI("========> FP_MSG_FINGER_DOWN , id = %d\n", finger_id);
        break;

    case FP_MSG_FINGER_UP:
        LOGI("========> FP_MSG_FINGER_UP , id = %d\n", finger_id);
        break;

    case FP_MSG_NO_LIVE:
        LOGI("========> FP_MSG_NO_LIVE , id = %d\n", finger_id);
        break;

    case FP_MSG_PROCESS_SUCCESS:
        LOGI("========> FP_MSG_PROCESS_SUCCESS , proc = %d\n", percent);
        break;

    case FP_MSG_PROCESS_FAILED:
        LOGI("========> FP_MSG_PROCESS_FAILED , proc = %d\n", percent);
        break;

    case FP_MSG_ENROLL_SUCCESS:
        LOGI("========> FP_MSG_ENROLL_SUCCESS , id = %d\n", finger_id);
        break;

    case FP_MSG_ENROLL_FAILED:
        LOGI("========> FP_MSG_ENROLL_FAILED , id = %d\n", finger_id);
        break;

    case FP_MSG_MATCH_SUCESS:
        LOGI("========> FP_MSG_MATCH_SUCESS , id = %d\n", finger_id);
        break;

    case FP_MSG_MATCH_FAILED:
        LOGI("========> FP_MSG_MATCH_FAILED , id = %d\n", finger_id);
        break;

    case FP_MSG_CANCEL:
        LOGI("========> FP_MSG_CANCEL\n");
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

    error = fingerprint_manager->enroll();
    if (error < 0) {
        LOGE("Failedl to enroll fingerprint\n");
        return -1;
    }

    return 0;
}

static int do_auth(void) {
    int error = 0;

    error = fingerprint_manager->authenticate();
    if (error < 0) {
        LOGE("Failed to auth fingerprint\n");
        return -1;
    }

    return 0;
}

static int do_delete(void) {
    char c;
    int error = 0;

restart:
    print_delete_tips();
    scanf("%c%*c", &c);
    int type = c - '0';
    if (type > DELETE_ALL || type < 0)
        goto restart;

    if (type == DELETE_BY_ID) {
restart2:
        print_delete_id_tips();
        scanf("%c%*c", &c);
        int id = c - '0';
        if (id < 0) {
            LOGE("Invalid id, retry\n");
            goto restart2;
        }

        error = fingerprint_manager->delete(id, type);
        if (error < 0)
            LOGE("Failed to delete finger by id %d\n", id);

    } else if (type == DELETE_ALL) {
        error = fingerprint_manager->delete(0, type);
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
    error = fingerprint_manager->post_power_on();
    if(error)
        LOGE("Failed to post power on\n");

    error = fingerprint_manager->reset();
    if (error)
        LOGE("Failed to reset fingerprint sensor\n");

    return 0;
}

static void do_work(void) {
    char c;
    int error = 0;

    for (;;) {
restart:
        print_tips();
        scanf("%c%*c", &c);

        int action = c - '0';
        if (action >= TEST_MAX || action < 0)
            goto restart;

        LOGI("Going to %s\n", test2string(action));

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
    fingerprint_manager->unregister_event_listener(event_listener);
    fingerprint_manager->cancel();
    fingerprint_manager->deinit();

    exit(1);
}

int main(int argc, char *argv[]) {
    int error = 0;

    power_manager = get_power_manager();
    fingerprint_manager = get_fingerprint_manager();
    signal_handler = _new(struct signal_handler, signal_handler);

    signal_handler->set_signal_handler(signal_handler, SIGINT, handle_signal);
    signal_handler->set_signal_handler(signal_handler, SIGQUIT, handle_signal);
    signal_handler->set_signal_handler(signal_handler, SIGTERM, handle_signal);

    error = fingerprint_manager->init();
    if (error < 0) {
        LOGE("Failed to init fingerprint sensor\n");
        //return -1;
    }

    fingerprint_manager->register_event_listener(event_listener);

    do_work();

    while (1)
        sleep(1000);

    return 0;
}
