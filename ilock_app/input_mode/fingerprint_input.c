#include <stdio.h>
#include <string.h>
#include <types.h>
#include <utils/log.h>
#include <utils/common.h>
#include <utils/file_ops.h>
#include <utils/signal_handler.h>
#include <utils/assert.h>
#include <utils/runnable/default_runnable.h>
#include <utils/runnable/pthread_wrapper.h>

#include <app_input_manager.h>
#include <fingerprint_input.h>

#define LOG_TAG                         "FingerPrintInput"

#define NAME_MAX                        (127)
/* #define FINGERPRINT_LIMIT               (50) */
/* #define VERSION_TRAIL                   (0)  */
/* #define VERSION_RELEASE                 (1)  */

#define AUTH_MAX_TIMES                  (5)
#define MAX_ENROLL_TIMES                (5)
#define MAX_ENROLL_FINGERS              (5)

enum {
    STATE_IDLE                          = 0,
    STATE_BUSY
};

typedef enum {
    DELETE_ALL                          = 0,
    DELETE_BY_ID,
} delete_type_t;

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static fingerprint_finger_id_t fingers[MAX_ENROLL_FINGERS];
static int version_code = -1;
static int work_state = STATE_IDLE;
static int enroll_steps;
static int auth_count;
static uint32_t finger_count = 0;

struct fingerprint_input *fingerprint_input;

static void set_state_idle(void)
{
    pthread_mutex_lock(&lock);

    work_state = STATE_IDLE;
    pthread_cond_signal(&cond);

    pthread_mutex_unlock(&lock);
}

#if 0
static void set_state_busy(void)
{
    pthread_mutex_lock(&lock);

    work_state = STATE_BUSY;
    pthread_cond_signal(&cond);

    pthread_mutex_unlock(&lock);
}

static void wait_state_idle(void)
{
    pthread_mutex_lock(&lock);

    while (work_state == STATE_BUSY)
        pthread_cond_wait(&cond, &lock);

    pthread_mutex_unlock(&lock);
}
#endif

static void ma_fp_callback(const fingerprint_msg_t *msg)
{
    switch (msg->type) {
    case FINGERPRINT_ERROR:
        LOGI("========> FINGERPRINT_ERROR: error=%d\n", msg->data.error);
        set_state_idle();
        break;

    case FINGERPRINT_ACQUIRED:
        LOGD("========> FINGERPRINT_ACQUIRED: acquired_info=%d\n",
                msg->data.acquired.acquired_info);

        if (msg->data.acquired.acquired_info == FINGERPRINT_ACQUIRED_TRIAL_OVER) {
            LOGW("================================\n");
            LOGW("          Trial Over!\n");
            LOGW("================================\n");
            return;
        }

        if (msg->data.acquired.acquired_info == FINGERPRINT_ACQUIRED_GOOD)
            LOGI("Finger good\n");

        else if (msg->data.acquired.acquired_info == FINGERPRINT_ACQUIRED_INSUFFICIENT)
            LOGW("Finger bad\n");

        else if (msg->data.acquired.acquired_info == FINGERPRINT_ACQUIRED_FINGER_DOWN)
            LOGI("Finger down\n");

        else if (msg->data.acquired.acquired_info == FINGERPRINT_ACQUIRED_FINGER_UP)
            LOGI("Finger up\n");

        else if (msg->data.acquired.acquired_info == FINGERPRINT_ACQUIRED_DUPLICATE_AREA)
            LOGW("Duplicate finger area\n");

        else if (msg->data.acquired.acquired_info == FINGERPRINT_ACQUIRED_IMAGER_DIRTY)
            LOGW("Sensor may be dirty\n");

        else if (msg->data.acquired.acquired_info == FINGERPRINT_ACQUIRED_LOW_COVER)
            LOGW("Sensor low cover\n");

        else if (msg->data.acquired.acquired_info == FINGERPRINT_ACQUIRED_DUPLICATE_FINGER) {
            LOGW("Duplicate finger! Canceling...\n");
            ma_fingerprint_cancel();
            set_state_idle();
        }

        break;

    case FINGERPRINT_AUTHENTICATED:
        LOGD("========> FINGERPRINT_AUTHENTICATED: fid=%d\n",
                msg->data.authenticated.finger.fid);

        if (msg->data.authenticated.finger.fid == 0) {
            if (auth_count >= AUTH_MAX_TIMES - 1) {
                LOGE("Finger auth failure! Canceling...\n");
                ma_fingerprint_cancel();
                auth_count = 0;
                set_state_idle();
            } else {
                auth_count++;
                LOGE("Finger auth failure! %d try left\n", AUTH_MAX_TIMES - auth_count);
            }

        } else {
            LOGI("Finger auth success, id=%d!\n", msg->data.authenticated.finger.fid);
            auth_count = 0;
            set_state_idle();
        }
        break;

    case FINGERPRINT_TEMPLATE_ENROLLING:
        LOGD("========> FINGERPRINT_TEMPLATE_ENROLLING: fid=%d, rem=%d\n",
                msg->data.enroll.finger.fid,
                msg->data.enroll.samples_remaining);

        if (enroll_steps == -1)
            enroll_steps = msg->data.enroll.samples_remaining;

        int progress = (enroll_steps + 1 - msg->data.enroll.samples_remaining);

        LOGI("Finger enrolling %d%%\n", 100 * progress / (enroll_steps + 1));

        if (msg->data.enroll.samples_remaining == 0)
            set_state_idle();

        break;

    case FINGERPRINT_TEMPLATE_REMOVED:
        LOGD("========> FINGERPRINT_TEMPLATE_REMOVED: fid=%d\n",
                msg->data.removed.finger.fid);
        LOGI("Finger removed, id=%d\n", msg->data.removed.finger.fid);
        set_state_idle();
        break;

    default:
        LOGE("Invalid msg type: %d", msg->type);
        break;
    }
}


static int fingerprint_input_enroll(struct input_dev_param *param)
{
    int error = 0;

    enroll_steps = -1;

    error = ma_fingerprint_enroll();
    if (error) {
        LOGE("Failedl to enroll fingerprint, fingers can't more than %d\n",
                MAX_ENROLL_FINGERS);
        return -1;
    }

    return 0;
}

static int fingerprint_input_authenticate(struct input_dev_param *param)
{
    int error = 0;

    error = ma_fingerprint_enumerate(fingers, &finger_count);
    if (error) {
        LOGE("Failed to enumerate fingers\n");
        return -1;
    }

    assert_die_if(finger_count > MAX_ENROLL_FINGERS, "Invalid finger size\n");

    if (finger_count <= 0) {
        LOGW("No any valid finger's templete\n");
        return -1;
    }

    error = ma_fingerprint_authenticate();
    if (error) {
        LOGE("Failed to auth fingerprint\n");
        return -1;
    }

    return 0;
}

static int fingerprint_input_delete(struct input_dev_param *param)
{
    int error = 0;
    int type = DELETE_ALL;

    if (type == DELETE_ALL) {
        error = ma_fingerprint_remove(type);
        if (error)
            LOGE("Failed to delete all finger\n");
    }

    return error;
}


static int fingerprint_input_init(void)
{
    int error = 0;
    char version_str[NAME_MAX];

    version_code = ma_fingerprint_get_version(version_str, sizeof(version_str));
    LOGI("%s\n", version_str);

    LOGI("=======================================\n");
    if (version_code == VERSION_TRAIL) {
        LOGI("Notice: This is debug version\n");

    } else if (version_code == VERSION_RELEASE) {
        LOGI("Notice: This is release version\n");

    } else {
        assert_die_if(1, "Unknown version\n");
    }
    LOGI("=======================================\n");

    error = ma_fingerprint_set_notify_callback(ma_fp_callback);
    if (error) {
        LOGE("Failed to set callback\n");
        return -1;
    }

    error = ma_fingerprint_open(get_user_system_dir(getuid() ) );
    if (error) {
        LOGE("Failed to open microarray lib\n");
        return -1;
    }

    error = ma_fingerprint_set_enroll_times(MAX_ENROLL_TIMES);
    if (error) {
        LOGE("Failed to set enroll times\n");
        return -1;
    }

    error = ma_fingerprint_set_max_fingers(MAX_ENROLL_FINGERS);
    if (error) {
        LOGE("Failed to set max fingers\n");
        return -1;
    }

    return 0;
}

static int fingerprint_input_deinit(void)
{

    return 0;
}



/* ============================== Public API end ============================ */

static struct input_manager this = {
        .init                       = fingerprint_input_init,
        .deinit                     = fingerprint_input_deinit,
        .enroll                     = fingerprint_input_enroll,
        .authenticate               = fingerprint_input_authenticate,
        .delete                     = fingerprint_input_delete,
#if 0
        .register_event_listener    = register_event_listener,
        .unregister_event_listener  = unregister_event_listener,

        .enroll                     = enroll,
        .authenticate               = authenticate,
        .delete                     = delete,
        .cancel                     = cancel,
        .reset                      = reset,
        .set_enroll_timeout         = set_enroll_timeout,
        .set_authenticate_timeout   = set_authenticate_timeout,
        .post_power_off             = post_power_off,
        .post_power_on              = post_power_on,
#endif
};

struct input_manager* get_fingerprint_input(void)
{
    return &this;
}

