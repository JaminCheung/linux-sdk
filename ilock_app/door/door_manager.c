#include <stdio.h>
#include <utils/log.h>
#include <utils/list.h>
#include <utils/assert.h>
#include <utils/common.h>
#include <utils/file_ops.h>
#include <utils/signal_handler.h>
#include <utils/assert.h>
#include <utils/runnable/default_runnable.h>
#include <utils/runnable/pthread_wrapper.h>
#include <door_manager.h>

#define LOG_TAG                         "DoorManager"

static pthread_mutex_t listener_lock = PTHREAD_MUTEX_INITIALIZER;
static LIST_HEAD(listener_list);
static struct default_runnable* door_runner;
static int door_manage_is_running = 0;




static void door_manager_thread(struct pthread_wrapper* thread, void* param)
{
    LOGI("====%s  === %d =======\n", __FUNCTION__, __LINE__);
    int door_state = 0;

    (void)door_state;

    while (door_manage_is_running) {

        LOGI("====%s  === %d =======\n", __FUNCTION__, __LINE__);
        usleep(1 * 1000 * 1000);

        pthread_mutex_lock(&listener_lock);


        pthread_mutex_unlock(&listener_lock);
    }
}

static int door_manager_door_state(void)
{
    LOGI("====%s  === %d =======\n", __FUNCTION__, __LINE__);

    return 0;
}


static int door_manager_ctrl_door_state(int state)
{
    LOGI("====%s  === %d =======\n", __FUNCTION__, __LINE__);

    return 0;
}

static void door_manager_register_door_listener(door_state_listener_t listener) {

    assert_die_if(listener == NULL, "listener is NULL\n");

    pthread_mutex_lock(&listener_lock);


    pthread_mutex_unlock(&listener_lock);
}

static void door_manager_unregister_door_listener(door_state_listener_t listener) {

    assert_die_if(listener == NULL, "listener is NULL\n");


    pthread_mutex_lock(&listener_lock);


    pthread_mutex_unlock(&listener_lock);
}



static int door_manager_deinit(void)
{
    LOGI("====%s  === %d =======\n", __FUNCTION__, __LINE__);
    door_manage_is_running = 0;
    door_runner->wait(door_runner);
    _delete(door_runner);
    return 0;
}

static int door_manager_init(void)
{
    LOGI("====%s  === %d =======\n", __FUNCTION__, __LINE__);
    door_runner = _new(struct default_runnable, default_runnable);
    door_runner->runnable.run = door_manager_thread;
    door_runner->set_thread_count(door_runner, 1);
    door_manage_is_running = 1;

    door_runner->start(door_runner, NULL);

    return 0;
}





/* ============================== Public API end ============================ */

static struct door_manager this = {
    .init                       = door_manager_init,
    .deinit                     = door_manager_deinit,
    .register_door_listener     = door_manager_register_door_listener,
    .unregister_door_listener   = door_manager_unregister_door_listener,
    .ctrl_door_state            = door_manager_ctrl_door_state,
    .get_door_state             = door_manager_door_state,
};

struct door_manager* get_door_manager(void)
{
    return &this;
}

