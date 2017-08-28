#include <stdio.h>
#include <errno.h>

#include <utils/log.h>
#include <utils/common.h>
#include <utils/runnable/default_runnable.h>
#include <utils/runnable/pthread_wrapper.h>
#include <timer/timer_manager.h>
#include <app_input_manager.h>
#include <app_power_manager.h>
#include <door_manager.h>

#define LOG_TAG                         "iLock APP Manager"

static struct input_manager     *input_manager;
static struct output_manager    *output_manager;
static struct door_manager      *door_manager;
static struct wifi_manager      *wifi_manager;
static struct wake_lock         lock_wake_lock;


static struct default_runnable *main_runner;
static struct default_runnable *normal_runner;
static struct default_runnable *admin_runner;


/*
 * resources
 * */
static struct app_power_manager *power_manager;

/************************timer callback****************************************/


/************************normal mode******************************************/

static int normal_authenticate_process(struct input_manager *input_manager)
{
    int auth_result = 0;
    struct input_dev_param param;

    /* this is blocked */
    auth_result = input_manager->authenticate(&param);
    if (AUTH_SUCCESS == auth_result) {

    }


    return auth_result;
}

static int normal_open_door(struct door_manager *door_manage)
{
    int ret = 0;

    door_manage->ctrl_door_state(DOOR_UNLOCKING);
    ret = door_manage->get_door_state();

    //ret = DOOR_UNLOCK_SUCCESS;

    return ret;
}


static int normal_wirless_update_status(struct wifi_manager *wifi_manager, int mode)
{

    return 0;
}

static int normal_hint_result(struct output_manager *output_manager, int mode)
{

    return 0;
}


static void normal_mode_thread(struct pthread_wrapper* thread, void* param)
{
    int ret = 0;
    int retry = 3;


    normal_hint_result(output_manager, HINT_NORMAL_WAKEUP);

    do {
        /*
         * this is blocked
         */
        ret = normal_authenticate_process(input_manager);
        if (ret >= AUTH_SUCCESS) {
            ret = normal_open_door(door_manager);
            normal_wirless_update_status(wifi_manager, ret);
        }

        normal_hint_result(output_manager, ret);
    } while (retry--);

}

/************************admin mode*******************************************/
static int admin_hint_result(struct output_manager *output_manager, int state)
{

    return 0;
}


static int admin_authenticate_process(struct input_manager *input_manager)
{
    int auth_result = 0;
    struct input_dev_param param;

    /* this is blocked */
    auth_result = input_manager->authenticate(&param);
    if (AUTH_SUCCESS == auth_result) {

    }

    return auth_result;
}

static int is_have_administrator(void)
{

    return 0;
}

static int admin_get_user_select(void)
{
    struct input_dev_param param;

    return input_manager->get_keycode(&param);
}

static int admin_add_delete_element(void)
{
    int ret = 0;
    /* wait for user select */
    ret = admin_get_user_select();

    if (ret == ADD_NEW_ELEMENT) {
        admin_hint_result(output_manager, ret);
    } else if (ret == DELETE_ELEMENT) {
        admin_hint_result(output_manager, ret);
    }

    return ret;
}

static void admin_mode_thread(struct pthread_wrapper* thread, void* param)
{
    int ret = 0;

    ret = is_have_administrator();
    if (ret) {
        /* already enroll administraort */
        ret = admin_authenticate_process(input_manager);
        admin_add_delete_element();
    } else {
        /* the administrator is not exist. now enrolling administrator */
    }
}

/************************door manager******************************************/
static void door_state_callback(int door_state)
{
    LOGI("====%s  === %d =====door state=[%d]=\n",
            __FUNCTION__, __LINE__, door_state);
}

/************************main mode*********************************************/
static void main_thread(struct pthread_wrapper* thread, void* param)
{
    LOGI("====%s  === %d =======\n", __FUNCTION__, __LINE__);
    int work_mode = 0;
    struct input_dev_param dev_param;

    while (1) {
        work_mode = input_manager->work_mode(&dev_param);
        if (work_mode == NORMAL_MODE) {
            //normal_hint_result(output_manager, HINT_NORMAL_WAKEUP);
            //nornal_mode_process();
            normal_runner->set_thread_count(normal_runner, 1);
            normal_runner->start(normal_runner, NULL);

        } else if (work_mode == ADMIN_MODE) {
            admin_runner->set_thread_count(normal_runner, 1);
            admin_runner->start(admin_runner, NULL);

        } else {
            LOGE("The work mode should be normal or admin mode. "
                    "Now work mode=%d.\n", work_mode);
        }
    } /* end of while(1) */
}




static int manager_initialization(void)
{
    //int ret = 0;
#if 1

#endif

#if 1
    power_manager = get_app_power_manager();
    if (power_manager) {
        power_manager->wake_lock_init(&lock_wake_lock, "power");
    }
#endif

#if 1
    input_manager = get_input_manager();
    if (input_manager) {
        input_manager->init();
    }
#endif

#if 0
    output_manager = get_output_manager();
    if (output_manager) {

    }
#endif

#if 0
    wifi_manager = get_wifi_manager();
    if (wifi_manager) {

    }
#endif

#if 0
    bt_manager = get_wifi_manager();
    if (bt_manager) {

    }
#endif

#if 1
    door_manager = get_door_manager();
    if (door_manager) {
        door_manager->init();
        door_manager->register_door_listener(door_state_callback);
    }
#endif

    return 0;
}

int main(int argc, char **argv)
{
    LOGE("Welcome to Ingenic Smart Security World\n");
    input_manager = NULL;

    manager_initialization();

    main_runner = _new(struct default_runnable, default_runnable);
    main_runner->runnable.run = main_thread;
    main_runner->set_thread_count(main_runner, 1);
    main_runner->start(main_runner, NULL);


    normal_runner = _new(struct default_runnable, default_runnable);
    normal_runner->runnable.run = normal_mode_thread;



    admin_runner = _new(struct default_runnable, default_runnable);
    admin_runner->runnable.run = admin_mode_thread;

    while(1) {
        usleep(1 * 1000 * 1000);
    }

    /*
     * blocked
     * */
    main_runner->wait(main_runner);
    normal_runner->wait(normal_runner);
    admin_runner->wait(admin_runner);

    _delete(normal_runner);
    _delete(admin_runner);
    _delete(main_runner);

    return 0;
}

