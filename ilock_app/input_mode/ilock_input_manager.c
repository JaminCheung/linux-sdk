#include <stdio.h>
#include <string.h>
#include <utils/log.h>
#include <utils/list.h>
#include <utils/assert.h>
#include <utils/common.h>
#include <utils/runnable/default_runnable.h>
#include <utils/runnable/pthread_wrapper.h>
#include <app_input_manager.h>

#define LOG_TAG                         "InputManager"

struct input_mode_listener{
    input_mode_device_t cb;
    struct list_head    node;
};


static struct default_runnable* enroll_runner;
static struct default_runnable* auth_runner;
static struct default_runnable* delete_runner;
static struct default_runnable* keycode_runner;
struct input_dev_param keycode_param;

static struct list_head unlock_mode_list;
static pthread_mutex_t unlock_mode_lock;

static LIST_HEAD(unlock_mode_list);
static int current_user_id;



static void input_enroll_thread(struct pthread_wrapper* thread, void* param)
{
    struct input_dev_param *enroll = (struct input_dev_param *)param;
    struct input_manager *input;

    while(1) {
        LOGI("====%s  === %d =======\n", __FUNCTION__, __LINE__);
        usleep(1 * 1000 * 1000);
    }

    pthread_mutex_lock(&enroll->queue_lock);

    list_for_each_entry(input, &unlock_mode_list, node) {
        LOGI("----------------------------------------\n");
        if (input->enroll) {
            enroll->enroll_result = input->enroll(enroll);
        }
    }

    pthread_cond_wait(&enroll->queue_ready, &enroll->queue_lock);

    pthread_mutex_unlock(&enroll->queue_lock);


}

static void input_authenticate_thread(struct pthread_wrapper* thread, void* param)
{
    struct input_dev_param *auth = (struct input_dev_param *)param;
    struct input_manager *input;

    while(1) {
        LOGI("====%s  === %d =======\n", __FUNCTION__, __LINE__);
        usleep(1 * 1000 * 1000);
    }

    pthread_mutex_lock(&auth->queue_lock);

    list_for_each_entry(input, &unlock_mode_list, node) {
        LOGI("----------------------------------------\n");
        if (input->authenticate)
            auth->auth_result = input->authenticate(auth);
    }

    pthread_cond_wait(&auth->queue_ready, &auth->queue_lock);

    pthread_mutex_unlock(&auth->queue_lock);

}


static void input_delete_thread(struct pthread_wrapper* thread, void* param)
{
    struct input_dev_param *del = (struct input_dev_param *)param;
    struct input_manager *input;

    while(1) {
        LOGI("====%s  === %d =======\n", __FUNCTION__, __LINE__);
        usleep(1 * 1000 * 1000);
    }

    pthread_mutex_lock(&del->queue_lock);

    list_for_each_entry(input, &unlock_mode_list, node) {
        LOGI("----------------------------------------\n");
        if (input->delete) {
            del->del_result = input->delete(del);
        }
    }

    pthread_cond_wait(&del->queue_ready, &del->queue_lock);

    pthread_mutex_unlock(&del->queue_lock);

}


static void input_keyboard_thread(struct pthread_wrapper* thread, void* param)
{
    struct input_dev_param *keyboard = (struct input_dev_param *)param;
    struct input_manager *input;

    while(1) {
        LOGI("====%s  === %d =======\n", __FUNCTION__, __LINE__);
        usleep(1 * 1000 * 1000);
    }

    pthread_mutex_lock(&keyboard->queue_lock);

    list_for_each_entry(input, &unlock_mode_list, node) {
        LOGI("====%s  === %d =======\n", __FUNCTION__, __LINE__);
        if (input->get_keycode) {
            keyboard->key_code = input->get_keycode(keyboard);
        }
    }

    pthread_cond_wait(&keyboard->queue_ready, &keyboard->queue_lock);

    pthread_mutex_unlock(&keyboard->queue_lock);
}


static void register_input_mode_device(input_mode_device_t listener) {

    assert_die_if(listener == NULL, "listener is NULL\n");

    struct input_mode_listener *l = NULL;

    pthread_mutex_lock(&unlock_mode_lock);

    list_for_each_entry(l, &unlock_mode_list, node) {
        if (l->cb == listener) {
            pthread_mutex_unlock(&unlock_mode_lock);
            return;
        }
    }

    l = malloc(sizeof(struct input_mode_listener));
    l->cb = listener;
    list_add_tail(&l->node, &unlock_mode_list);

    pthread_mutex_unlock(&unlock_mode_lock);
}

#if 0
static void unregister_input_mode_device(input_mode_device_t listener) {

    assert_die_if(listener == NULL, "listener is NULL\n");

    struct input_mode_listener *l, *nl;

    pthread_mutex_lock(&unlock_mode_lock);

    list_for_each_entry_safe(l, nl, &unlock_mode_list, node) {
        if (l->cb == listener) {
            list_del(&l->node);

            free(l);

            pthread_mutex_unlock(&unlock_mode_lock);

            return;
        }
    }

    pthread_mutex_unlock(&unlock_mode_lock);
}
#endif



static int input_manager_enroll(struct input_dev_param *dev_params)
{
    struct input_dev_param param;

    pthread_mutex_init(&(param.queue_lock), NULL);
    pthread_cond_init(&(param.queue_ready), NULL);

    if (enroll_runner) {
        enroll_runner->set_thread_count(enroll_runner, 1);
        enroll_runner->start(enroll_runner, &param);
    }
    enroll_runner->wait(enroll_runner);

    pthread_cond_destroy(&(param.queue_ready));
    pthread_mutex_destroy(&(param.queue_lock));

    return param.enroll_result;
}



static int input_manager_authenticate(struct input_dev_param *dev_params)
{
    struct input_dev_param param;

    pthread_mutex_init(&(param.queue_lock), NULL);
    pthread_cond_init(&(param.queue_ready), NULL);

    if (auth_runner) {
        auth_runner->set_thread_count(auth_runner, 1);
        auth_runner->start(auth_runner, &param);
    }
    auth_runner->wait(auth_runner);

    pthread_cond_destroy(&(param.queue_ready));
    pthread_mutex_destroy(&(param.queue_lock));

    return param.auth_result;
}


static int input_manager_delete(struct input_dev_param *dev_params)
{
    struct input_dev_param param;

    pthread_mutex_init(&(param.queue_lock), NULL);
    pthread_cond_init(&(param.queue_ready), NULL);


    if (delete_runner) {
        delete_runner->set_thread_count(delete_runner, 1);
        delete_runner->start(delete_runner, &param);
    }
    delete_runner->wait(delete_runner);

    pthread_cond_destroy(&(param.queue_ready));
    pthread_mutex_destroy(&(param.queue_lock));

    return param.del_result;
}

static int input_manager_get_keycode(struct input_dev_param *dev_params)
{
    struct input_dev_param param;


    if (keycode_runner) {
        keycode_runner->set_thread_count(delete_runner, 1);
        keycode_runner->start(keycode_runner, &param);
    }

    /* blocked */
    keycode_runner->wait(keycode_runner);

    dev_params->key_code  = param.key_code;
    dev_params->key_value = param.key_value;


    return 0;
}


static int input_manager_init(void)
{
    current_user_id = getuid();
    INIT_LIST_HEAD(&unlock_mode_list);

    pthread_mutex_init(&unlock_mode_lock, NULL);


    enroll_runner = _new(struct default_runnable, default_runnable);
    enroll_runner->runnable.run = input_enroll_thread;

    auth_runner = _new(struct default_runnable, default_runnable);
    auth_runner->runnable.run = input_authenticate_thread;

    delete_runner = _new(struct default_runnable, default_runnable);
    delete_runner->runnable.run = input_delete_thread;

    /*
     * keycode thread shold be allways running
     * may shold find other way to detect keyboard
     */
    keycode_runner = _new(struct default_runnable, default_runnable);
    keycode_runner->runnable.run = input_keyboard_thread;
    pthread_mutex_init(&(keycode_param.queue_lock), NULL);
    pthread_cond_init(&(keycode_param.queue_ready), NULL);


    /* intput device info init */
    get_keyboard_input()->init();
    get_fingerprint_input()->init();


    //keyboard_input_initialization();
    //fingerprint_input_initialization();
    register_input_mode_device( get_keyboard_input() );
    register_input_mode_device( get_fingerprint_input() );

    return 0;
}

static int input_manager_deinit(void)
{
    pthread_cond_destroy(&(keycode_param.queue_ready));
    pthread_mutex_destroy(&(keycode_param.queue_lock));

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



static int input_manager_get_workmode(struct input_dev_param *param)
{
    return NORMAL_MODE;
}




/* ============================== Public API end ============================ */

static struct input_manager this = {
        .init                       = input_manager_init,
        .deinit                     = input_manager_deinit,
        .enroll                     = input_manager_enroll,
        .authenticate               = input_manager_authenticate,
        .delete                     = input_manager_delete,
        .work_mode                  = input_manager_get_workmode,
        .get_keycode                = input_manager_get_keycode,
};

struct input_manager* get_input_manager(void)
{
    return &this;
}

