#include <types.h>
#include <utils/log.h>
#include <utils/common.h>
#include <fingerprint/fingerprint_manager.h>

#define LOG_TAG "test_fingerprint"

enum {
    ENROLL_TEST,
    CANCEL_ENROLL,
    AUTH_TEST,
    CANCEL_AUTH,
    LIST_FPS
};

static struct fingerprint_manager* fingerprint_manager;

static void on_removal_error(struct fingerprint* fp, int error_code,
        const char* error_string) {

}

static void on_removal_successed(struct fingerprint* fp) {

}

static struct removal_callback removal_callback = {
        .on_removal_error = on_removal_error,
        .on_removal_successed = on_removal_successed,
};

static void on_enrollment_error(int error_code, const char* error_string) {

}

static void on_enrollment_help(int help_code, const char* help_string) {

}

static void on_enrollment_progress(int remaining) {

}

static struct enrollment_callback enrollment_callback = {
        .on_enrollment_error = on_enrollment_error,
        .on_enrollment_help = on_enrollment_help,
        .on_enrollment_progress = on_enrollment_progress,
};

static void on_auth_error(int error_code, const char* error_string) {

}

static void on_auth_help(int help_code, const char* help_string) {

}

static void on_auth_successed(struct authentication_result* result) {

}

static void on_auth_failed(void) {

}

static void on_auth_acquired(int acquired_info) {

}

static struct authentication_callback auth_callback = {
        .on_authentication_acquired = on_auth_acquired,
        .on_authentication_error = on_auth_error,
        .on_authentication_failed = on_auth_failed,
        .on_authentication_help = on_auth_help,
        .on_authentication_successed = on_auth_successed,
};

static void do_enroll(void) {
    LOGI("=====> %s <=====\n", __FUNCTION__);
}

static void do_cancel_enroll(void) {
    LOGI("=====> %s <=====\n", __FUNCTION__);
}

static void do_auth(void) {
    LOGI("=====> %s <=====\n", __FUNCTION__);
}

static void do_cancel_auth(void) {
    LOGI("=====> %s <=====\n", __FUNCTION__);
}

static void do_list_fps(void) {
    LOGI("=====> %s <=====\n", __FUNCTION__);
}

static void print_actions(void) {
    fprintf(stderr, "==============================================\n");
    fprintf(stderr, "  %d.Enroll\n", ENROLL_TEST);
    fprintf(stderr, "  %d.Cancel enroll\n", CANCEL_ENROLL);
    fprintf(stderr, "  %d.Authenticate\n", AUTH_TEST);
    fprintf(stderr, "  %d.Cancel authentication\n", CANCEL_AUTH);
    fprintf(stderr, "  %d.List enrolled fingerprints\n", LIST_FPS);
    fprintf(stderr, "==============================================\n");
}

static void do_word(void) {
    char c;

restart:
    print_actions();
    scanf("%c%*c",&c);

    int action = c - '0';
    if (action > LIST_FPS || action < 0)
        goto restart;

    switch(action) {
    case ENROLL_TEST:
        do_enroll();
        break;

    case CANCEL_ENROLL:
        do_cancel_enroll();
        break;

    case AUTH_TEST:
        do_auth();
        break;

    case CANCEL_AUTH:
        do_cancel_auth();
        break;

    case LIST_FPS:
        do_list_fps();
        break;

    default:
        break;
    }
}

int main(int argc, char *argv[]) {
    int error = 0;

    do_word();

    fingerprint_manager = get_fingerprint_manager();

    error = fingerprint_manager->init();
    if (error < 0) {
        LOGE("Failed to init fingerprint manager\n");
        return -1;
    }

    if (!fingerprint_manager->is_hardware_detected()) {
        LOGE("Hardware not support any fingerprint sensor!\n");
        return -1;
    }


    return 0;
}
