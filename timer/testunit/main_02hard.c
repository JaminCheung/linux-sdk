#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
# include <unistd.h>
# include <utime.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <types.h>
#include <sys/time.h>
#include <utils/log.h>
#include <libqrcode_api.h>

#define LOG_TAG  "test_timer"
/* 每个定时器: 定时周期, 单位:ms  */
#define TEST_TIMER_INTERVAL_MS                    50
/* 将要申请的定时器个数, 最大值为TIMER_DEFAULT_MAX_CNT */
#define TEST_TIMER_REQUEST_CNT                   5
/* 每个定时器: 测试监控多少个interal定时周期 */
#define TEST_TIMER_MONITER_INTERVAL_CNT   50
/* 每个定时器:监控总时长 */
#define TEST_TIMER_MONITER_TIME   (TEST_TIMER_INTERVAL_MS * TEST_TIMER_MONITER_INTERVAL_CNT)

struct timer_display {
    char stat[TEST_TIMER_MONITER_INTERVAL_CNT][200];
    unsigned long long time_ms[TEST_TIMER_MONITER_INTERVAL_CNT];
    int interval_index;
};

pthread_mutex_t timer_func_lock = PTHREAD_MUTEX_INITIALIZER;
struct timer_display timer_dis[TEST_TIMER_REQUEST_CNT+1];
static struct timer_manager *timer_manager;

static void msleep(int ms)
{
    usleep(ms * 1000);
}

// void dump_timer_status(void)
// {
//     int i, j;
//     printf("-----Dumping display info-----\n");
//     for (i = 1 ; i <= TEST_TIMER_REQUEST_CNT; i++) {
//         printf("===== timer %d======\n", i);

//         for (j = 0 ; j < TEST_TIMER_MONITER_INTERVAL_CNT; j++) {
//             printf("s%d: %s\n", j, timer_dis[i].stat[j]);
//         }
//     }
// }

// void analysis_timer_data(void) {
//     unsigned long long ms = 0, ms_next = 0;
//     struct timer_display *data;
//     int i, j;

//     for (i = 1; i <= TEST_TIMER_REQUEST_CNT; i++) {
//         printf("===== timer%d  analysising======\n", i);
//         data = &timer_dis[i];
//         for (j = 0 ; j < TEST_TIMER_MONITER_INTERVAL_CNT - 1; j++) {
//             ms = data->time_ms[j];
//             ms_next = data->time_ms[j+1];
//             if (!ms || !ms_next)
//                 break;
//             if ((ms + TEST_TIMER_INTERVAL_MS)
//                     != ms_next) {
//                 printf("wrong at data index %d\n", j);
//                 printf("index%d ms = %llu\n", j, ms);
//                 printf("index%d ms = %llu\n", j+1,  ms_next);
//                 // dump_timer_status();
//                 return;
//             }
//         }
//         if (j == TEST_TIMER_MONITER_INTERVAL_CNT - 1)
//             printf("done\n");
//     }
//     return;
// }

void get_format_time_str(char *tstr, unsigned long long *ms)
{
    struct timeval val;
    gettimeofday(&val, NULL);
    sprintf(tstr, "%u:%u", (unsigned int)val.tv_sec, (unsigned int)val.tv_usec);
    *ms = val.tv_sec * 1000 + val.tv_usec / 1000;
}

void timer_callback(void *args) {
    int id = *(int*)args;
    struct timer_display *dis = &timer_dis[id];
    char tstr[200];
    unsigned long long ms;

    pthread_mutex_lock(&timer_func_lock);
    LOGI("%s: %d: id = %d\n", __func__, __LINE__, id);
    if (dis->interval_index >= (TEST_TIMER_MONITER_INTERVAL_CNT - 1)) {
        pthread_mutex_unlock(&timer_func_lock);
        return;
    }
    get_format_time_str(tstr, &ms);
    strcpy(dis->stat[dis->interval_index], tstr);
    dis->time_ms[dis->interval_index] = ms;
    dis->interval_index++;
    pthread_mutex_unlock(&timer_func_lock);
}

 int main(int argc, char *argv[]) {
    int *timer_arg = NULL;
    int *id_list = NULL;
    int i = -1, j= -1;
    int ret = 0;

    timer_manager = get_timer_manager();

    timer_arg = (int *) malloc(sizeof (int) * TEST_TIMER_REQUEST_CNT + 1);
    if (timer_arg == NULL) {
        LOGE("Cannot alloc more memory\n");
        goto out;
    }
    id_list = (int *) malloc(sizeof (int) * TEST_TIMER_REQUEST_CNT + 1);
    if (id_list == NULL) {
        LOGE("Cannot alloc more memory\n");
        goto out;
    }

    LOGI("timer total %d, timer interval %d ms\n",
            TEST_TIMER_REQUEST_CNT, TEST_TIMER_INTERVAL_MS);
    /* 测试项<初始化测试>: 初始化所有timer */
    for (i = 1; i <= TEST_TIMER_REQUEST_CNT; i++) {
        timer_arg[i] = i;
        LOGI("=====create timer %d with arg %d======\n", i, timer_arg[i]);
        id_list[i] = timer_manager->init(id_list[i], TEST_TIMER_INTERVAL_MS,
            0, timer_callback, &timer_arg[i]);
        if (id_list[i] < 0) {
            LOGE("Cannot call timer init on timer%d\n", i);
            ret = -1;
            goto out;
        }
    }

    /* 测试项<启动测试>: 启动所有timer */
    LOGI("=====enable all the times======\n");
    for (j = 1; j <= TEST_TIMER_REQUEST_CNT; j++) {
        ret = timer_manager->start(id_list[j]);
        if (ret < 0) {
            LOGE("Cannot call timer start on timer%d\n", i);
            goto out;
        }
    }

    /* 测试项<运行测试>: 观察终端的各个timer处理函数输出 */
    LOGI("wait for %d miliseconds for observation on timer handling\n", TEST_TIMER_MONITER_TIME);
    msleep(TEST_TIMER_MONITER_TIME);

    i--;
    j--;
out:
    if (i < 0 || j < 0) {
        ret = -1;
        goto go_away;
    }

    /* 测试项<资源回收>: 主程序退出前，关闭所有已经打开的定时器 */
    for (;j >= 1; j--) {
        LOGI("=====disable timer %d======\n", j);
        ret = timer_manager->stop(id_list[j]);
        if (ret < 0) {
            LOGE("Cannot call timer disable on timer%d\n", j);
            while(1);
        }
    }

    /* 测试项<资源回收>: 主程序退出前，销毁所有已经打开的定时器 */
    for (;i >= 1; i--) {
        LOGI("=====destroy timer %d======\n", i);
        ret = timer_manager->deinit(id_list[i]);
        if (ret < 0) {
            LOGE("Cannot call timer deinit on timer%d\n", i);
            while(1);
        }
    }

    // dump_timer_status();
    // analysis_timer_data();

go_away:
    if (timer_arg) {
        free(timer_arg);
        timer_arg = NULL;
    }
    if (id_list) {
        free(id_list);
        id_list = NULL;
    }
    return ret;
 }