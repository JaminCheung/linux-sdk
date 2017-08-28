#include <stdio.h>
#include <string.h>
#include<linux/input.h>
#include <utils/log.h>
#include <utils/common.h>
#include <utils/runnable/default_runnable.h>
#include <utils/runnable/pthread_wrapper.h>

#include <app_input_manager.h>
#include <keyboard_input.h>
#include <cypress/cypress_manager.h>

#define LOG_TAG                         "KeyBoardInput"
#define PASSWORD_LENGTH                 (32)

static unsigned char password[PASSWORD_LENGTH];
static unsigned int password_num;

static struct cypress_manager           *cypress;

//static pthread_cond_t local_cond = PTHREAD_COND_INITIALIZER;
//static pthread_mutex_t local_keyboard_mutex = PTHREAD_MUTEX_INITIALIZER;
static int flag_confirm = 0;
static int flag_password = 0;

static struct default_runnable* enroll_runner;
static struct default_runnable* auth_runner;
static struct default_runnable* delete_runner;
static struct default_runnable* keycode_runner;

struct input_dev_param *input_manager_param;
struct input_dev_param *keycode_param;

/*
 * Functions callback
 */
static void keys_report_handler(uint8_t keycode, uint8_t keyvalue)
{

    switch (keycode) {
    case KEY_LEFT:  /* '*' */
        break;

    case KEY_RIGHT: /* '#' */
        flag_confirm = 1;
        break;

    case KEY_POWER:
        break;

    default:
        if (flag_password && password_num < PASSWORD_LENGTH) {
            password[password_num++] = keycode;
        }
        break;
    }



    /*
     * notify the main thread, there is key detected
     */
    if (keycode_param) {
        pthread_mutex_lock(&keycode_param->queue_lock);

        keycode_param->key_code = keycode;
        keycode_param->key_value = keyvalue;
        pthread_cond_signal(&keycode_param->queue_ready);

        pthread_mutex_unlock(&keycode_param->queue_lock);
    }

    /*
     * notify the input manager
     */
    if (flag_confirm && input_manager_param) {
        pthread_mutex_lock(&input_manager_param->queue_lock);
        flag_confirm = 0;
        memcpy(input_manager_param->content, password, sizeof(password));
        memset(password, 0x00, sizeof(password) );
        pthread_cond_signal(&input_manager_param->queue_ready);

        pthread_mutex_unlock(&input_manager_param->queue_lock);
    }



}


static void card_report_handler(int dev_fd)
{

}



static void keyboard_input_enroll_thread(struct pthread_wrapper* thread, void* param)
{
    struct input_dev_param *dev_param = (struct input_dev_param *)param;
    struct input_manager *input;
    flag_password = 1;

    (void)input;
    (void)dev_param;

    flag_password = 0;
}

static void keyboard_input_authenticate_thread(struct pthread_wrapper* thread, void* param)
{
    struct input_dev_param *dev_param = (struct input_dev_param *)param;
    struct input_manager *input;

    flag_password = 1;


    (void)input;
    (void)dev_param;

    flag_password = 0;

}

static void keyboard_input_delete_thread(struct pthread_wrapper* thread, void* param)
{
    struct input_dev_param *dev_param = (struct input_dev_param *)param;
    struct input_manager *input;
    flag_password = 1;

    (void)input;
    (void)dev_param;

    flag_password = 0;
}

static void keyboard_input_keycode_thread(struct pthread_wrapper* thread, void* param)
{
    struct input_dev_param *dev_param = (struct input_dev_param *)param;
    struct input_manager *input;


    (void)input;
    (void)dev_param;

}

static int keyboard_input_enroll(struct input_dev_param *param)
{
    if (enroll_runner) {
        enroll_runner->set_thread_count(enroll_runner, 1);
        enroll_runner->start(enroll_runner, param);
    }

    return 0;
}

static int keyboard_input_authenticate(struct input_dev_param *param)
{
    if (auth_runner) {
        auth_runner->set_thread_count(auth_runner, 1);
        auth_runner->start(auth_runner, param);
    }

    return 0;
}

static int keyboard_input_delete(struct input_dev_param *param)
{

    if (delete_runner) {
        delete_runner->set_thread_count(delete_runner, 1);
        delete_runner->start(delete_runner, param);
    }

    return 0;
}

static int keyboard_input_get_keycode(struct input_dev_param *param)
{
    if (keycode_param == NULL) {
        keycode_param = (struct input_dev_param *)param;
    }

    if (keycode_runner) {
        keycode_runner->set_thread_count(keycode_runner, 1);
        keycode_runner->start(keycode_runner, param);
    }


    return 0;
}

static int keyboard_input_init(void)
{
    cypress = get_cypress_manager();

    enroll_runner = _new(struct default_runnable, default_runnable);
    enroll_runner->runnable.run = keyboard_input_enroll_thread;

    auth_runner = _new(struct default_runnable, default_runnable);
    auth_runner->runnable.run = keyboard_input_authenticate_thread;

    delete_runner = _new(struct default_runnable, default_runnable);
    delete_runner->runnable.run = keyboard_input_delete_thread;

    keycode_runner = _new(struct default_runnable, default_runnable);
    keycode_runner->runnable.run = keyboard_input_keycode_thread;


    if (cypress->init(keys_report_handler, card_report_handler)) {
        printf("Failed to init cypress manager.\n");
        return -1;
    }

    return 0;
}

static int keyboard_input_deinit(void)
{
    if (keycode_runner) {
        keycode_runner->wait(keycode_runner);
        _delete(keycode_runner);
    }

    if (delete_runner) {
        delete_runner->wait(delete_runner);
        _delete(delete_runner);
    }

    if (auth_runner) {
        auth_runner->wait(auth_runner);
        _delete(auth_runner);
    }

    if (enroll_runner) {
        enroll_runner->wait(enroll_runner);
        _delete(enroll_runner);
    }

    return 0;
}

/* ============================== Public API end ============================ */

static struct input_manager this = {
        .init                       = keyboard_input_init,
        .deinit                     = keyboard_input_deinit,
        .enroll                     = keyboard_input_enroll,
        .authenticate               = keyboard_input_authenticate,
        .delete                     = keyboard_input_delete,
#if 0
        .register_event_listener    = register_event_listener,
        .unregister_event_listener  = unregister_event_listener,


        .cancel                     = cancel,
        .reset                      = reset,
        .set_enroll_timeout         = set_enroll_timeout,
        .set_authenticate_timeout   = set_authenticate_timeout,
        .post_power_off             = post_power_off,
        .post_power_on              = post_power_on,
#endif
        .get_keycode                = keyboard_input_get_keycode,
};


struct input_manager* get_keyboard_input(void)
{
    return &this;
}

