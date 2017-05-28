#include <unistd.h>

#include <types.h>
#include <utils/log.h>
#include <alarm/alarm_manager.h>

#define LOG_TAG "test_alarm"


static struct alarm_manager* alarm_manager;

static void alarm_listener1(void) {
    LOGI("=====> %s ring <=====\n", __FUNCTION__);
}

static void alarm_listener2(void) {
    LOGI("=====> %s ring <=====\n", __FUNCTION__);
}

static void alarm_listener3(void) {
    LOGI("=====> %s ring <=====\n", __FUNCTION__);
}

int main(int argc, char *argv[]) {
    alarm_manager = get_alarm_manager();

    alarm_manager->init();

    uint64_t cur_timems = alarm_manager->get_current_time_ms();

    alarm_manager->set(cur_timems + 5 * 1000, alarm_listener1);
    alarm_manager->set(cur_timems + 15 * 1000, alarm_listener2);
    alarm_manager->set(cur_timems + 10 * 1000, alarm_listener3);

    sleep(1);

    alarm_manager->cancel(alarm_listener1);

    while(1)
        sleep(1000);

    alarm_manager->deinit();

    return 0;
}
