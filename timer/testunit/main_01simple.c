#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <utime.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <types.h>
#include <sys/time.h>
#include <utils/log.h>
#include <libqrcode_api.h>

#define LOG_TAG  "test_timer"
/* 每个定时器: 定时周期, 单位:ms  */
#define TEST_TIMER_INTERVAL_MS    75
/* 将要申请的定时器个数, 最大值为TIMER_DEFAULT_MAX_CNT */
#define TEST_TIMER_REQUEST_CNT   1
/* 每个定时器: 测试监控多少个interal定时周期 */
#define TEST_TIMER_MONITOR_INTERVAL_CNT   50
/* 每个定时器:监控总时长 */
#define TEST_TIMER_MONITOR_TIME   (TEST_TIMER_INTERVAL_MS * TEST_TIMER_MONITOR_INTERVAL_CNT)
/*是否只执行一次定时计数*/
#define TEST_TIMER_ONE_TIME_COUNTER   0  //1: one time counter  0: loop counter

static struct timer_manager *timer_manager;
static pthread_mutex_t timer_func_lock = PTHREAD_MUTEX_INITIALIZER;

static void msleep(int ms)
{
    usleep(ms * 1000);
}

static void timer_callback(void *args) {
    pthread_mutex_lock(&timer_func_lock);
    LOGI("%s: %d\n", __func__, __LINE__);
    pthread_mutex_unlock(&timer_func_lock);
}

 int main(int argc, char *argv[]) {
    timer_manager = get_timer_manager();
    int timer_id, timer_arg, ret;

    timer_id = 1;
    timer_arg = timer_id -1;
    LOGI("====Init one timer at id %d with interval %d ms, timer once counter is %d=====\n",
            timer_id, TEST_TIMER_INTERVAL_MS, TEST_TIMER_ONE_TIME_COUNTER);
    ret = timer_manager->init(timer_id,
                                    TEST_TIMER_INTERVAL_MS,
                                    TEST_TIMER_ONE_TIME_COUNTER,
                                    timer_callback,
                                    &timer_arg);
    if (ret < 0 || (ret != timer_id)) {
        LOGE("Cannot call timer manager init function\n");
        return -1;
    }
    ret = timer_manager->start(timer_id);
    if (ret < 0) {
        LOGE("Cannot call timer manager start function\n");
        return -1;
    }
    LOGI("wait for %d miliseconds\n", TEST_TIMER_MONITOR_TIME);
    msleep(TEST_TIMER_MONITOR_TIME);

    if (TEST_TIMER_ONE_TIME_COUNTER == 0) {
        LOGI("====Get counter test=====\n");
        for (int i = 0; i < 120; i++) {
            int64_t counter = timer_manager->get_counter(timer_id);
            LOGI("ms = %lld\n", counter);
            msleep(10);
        }
    }

    ret = timer_manager->stop(timer_id);
    if (ret < 0) {
        LOGE("Cannot call timer manager stop function\n");
        return -1;
    }
    LOGI("After timer stop wait for 2s\n");
    sleep(2);

    LOGI("====Release timer %d=====\n", timer_id);
    ret = timer_manager->deinit(timer_id);
    if (ret < 0) {
        LOGE("Cannot call timer manager deinit function\n");
        return -1;
    }
    return 0;
 }
