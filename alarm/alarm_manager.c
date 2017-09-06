/*
 *  Copyright (C) 2017, Zhang YanMing <yanmin.zhang@ingenic.com, jamincheung@126.com>
 *
 *  Ingenic Linux plarform SDK project
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under  the terms of the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>

#include <types.h>
#include <thread/thread.h>
#include <utils/log.h>
#include <utils/list.h>
#include <utils/assert.h>
#include <utils/common.h>
#include <alarm/alarm_manager.h>

static pthread_mutex_t alarms_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t init_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t start_lock = PTHREAD_MUTEX_INITIALIZER;

static LIST_HEAD(alarm_list);
static LIST_HEAD(trigger_list);

struct alarm {
    int type;
    int count;
    uint64_t when;
    int repeat_interval;
    alarm_listener_t listener;
    struct list_head head;
};

typedef enum {
    ANDROID_ALARM_RTC_WAKEUP,
    ANDROID_ALARM_RTC,
    ANDROID_ALARM_ELAPSED_REALTIME_WAKEUP,
    ANDROID_ALARM_ELAPSED_REALTIME,
    ANDROID_ALARM_SYSTEMTIME,

    ANDROID_ALARM_TYPE_COUNT,

} android_alarm_type_t;

#define ANDROID_ALARM_CLEAR(type)         _IO('a', 0 | ((type) << 4))
#define ANDROID_ALARM_WAIT                _IO('a', 1)
#define ANDROID_ALARM_SET(type)          _IOW('a', 2 | ((type) << 4), struct timespec)
#define ANDROID_ALARM_SET_AND_WAIT(type) _IOW('a', 3 | ((type) << 4), struct timespec)
#define ANDROID_ALARM_GET_TIME(type)     _IOW('a', 4 | ((type) << 4), struct timespec)
#define ANDROID_ALARM_SET_RTC            _IOW('a', 5, struct timespec)
#define ANDROID_ALARM_SET_TIMEZONE       _IOW('a', 6, struct timezone)

#define ANDROID_ALARM_BASE_CMD(cmd)      (cmd & ~(_IOC(0, 0, 0xf0, 0)))
#define ANDROID_ALARM_IOCTL_TO_TYPE(cmd) (_IOC_NR(cmd) >> 4)

#define LOG_TAG "alarm_manager"

static const int alarm_type = ANDROID_ALARM_RTC_WAKEUP;
static int fd;
static uint32_t init_count;
static uint32_t start_count;
static struct thread* thread;

__attribute__((unused)) static void dump_alarm_list_locked(void) {
    struct list_head* pos;

    LOGI("========================================\n");
    LOGI("Dump alarms\n");
    list_for_each(pos, &alarm_list) {
        struct alarm* alarm = list_entry(pos, struct alarm, head);
        LOGI("when: %lld\n", alarm->when);

    }
    LOGI("========================================\n");
}

static void dummy_listener(void) {

}

static uint64_t get_sys_time_ms(void) {
    struct timeval tv;
    struct timespec ts;
    int error = 0;

    error = ioctl(fd, ANDROID_ALARM_GET_TIME(ANDROID_ALARM_RTC_WAKEUP), &ts);
    if (error < 0) {
        gettimeofday(&tv, NULL);
        return ((uint64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000) + 0.5;
    }

    return ((uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000) + 0.5;
}

static void remove_alarm_locked(alarm_listener_t listener) {
    if (!list_empty(&alarm_list)) {
        struct list_head* pos;
        struct list_head* next_pos;

        list_for_each_safe(pos, next_pos, &alarm_list) {
            struct alarm* alarm = list_entry(pos, struct alarm, head);

            if (alarm->listener == listener) {
                list_del(&alarm->head);
                free(alarm);
            }
        }
    }
}

static int add_alarm_locked(struct alarm* alarm) {
    int size = 0;
    int index = 0;

    list_add_tail(&alarm->head, &alarm_list);

    size = get_list_size_locked(&alarm_list);

    /*
     * Sort the alarm trigger time
     */
    struct list_head* pos;
    struct list_head* pos_next;
    int i = size;
    while (i > 0) {
        list_for_each(pos, &alarm_list) {
            pos_next = pos->next;
            if (pos_next == &alarm_list)
                continue;

            struct alarm* alarm_prev = list_entry(pos, struct alarm, head);
            struct alarm* alarm_next = list_entry(pos_next, struct alarm, head);

            if (alarm_prev->when > alarm_next->when) {
                pos_next->prev = pos->prev;
                pos_next->next->prev = pos;
                pos->next = pos_next->next;
                pos_next->next = pos;
                pos->prev->next = pos_next;
                pos->prev = pos_next;
            }
        }

        i--;
    }

    /*
     * Find new alarm index in list
     */
    list_for_each(pos, &alarm_list) {
        struct alarm* alarm_cur = list_entry(pos, struct alarm, head);
        if (alarm->when == alarm_cur->when)
            break;
        index++;
    }

    LOGI("Adding alarm(when=%lld) at index %d.\n", alarm->when, index);

    return index;
}

static int wait_for_alarm(void) {
    int error = 0;

    do {
        error = ioctl(fd, ANDROID_ALARM_WAIT);
    } while (error < 0 && errno == EINTR);

    if (error < 0) {
        LOGE("Unable to wait on alarm: %s\n", strerror(errno));
        return -1;
    }

    return error;
}

static int set_alarm_dev(int type, uint64_t seconds, uint64_t nanoseconds) {
    struct timespec ts;

    ts.tv_sec = seconds;
    ts.tv_nsec = nanoseconds;

    int error = ioctl(fd, ANDROID_ALARM_SET(type), &ts);
    if (error < 0) {
        LOGE("Unable to set alarm to %lld.%09lld: %s\n", seconds, nanoseconds,
                strerror(errno));
        return -1;
    }

    return 0;
}

static void set_locked(struct alarm* alarm) {
    int seconds, nanoseconds;

    if (alarm->when < 0) {
        seconds = 0;
        nanoseconds = 0;
    } else {
        seconds = alarm->when / 1000;
        nanoseconds = (alarm->when % 1000) * 1000 * 1000;
    }

#ifdef LOCAL_DEBUG
    char buf[64];
    struct tm tm;
    uint64_t now = get_sys_time_ms();

    time_t now_s = now / 1000;
    gmtime_r(&now_s, &tm);
    asctime_r(&tm, buf);
    LOGI("Now:       %s", buf);

    time_t when_s = seconds + nanoseconds / 1000000000;
    gmtime_r(&when_s, &tm);
    asctime_r(&tm, buf);
    LOGI("Requested: %s", buf);
#endif

    set_alarm_dev(alarm->type, seconds, nanoseconds);
}

static void set_repeat(uint64_t when, int interval,
        alarm_listener_t listener) {
    assert_die_if(listener == NULL, "Listener is NULL\n");

    pthread_mutex_lock(&alarms_lock);

    struct alarm *alarm = calloc(1, sizeof(struct alarm));
    alarm->type = alarm_type;
    alarm->when = when;
    alarm->repeat_interval = interval;
    alarm->listener = listener;

    remove_alarm_locked(listener);
    int index = add_alarm_locked(alarm);
    if (index == 0)
        set_locked(alarm);

    pthread_mutex_unlock(&alarms_lock);
}

static struct alarm* create_triggered_alarm(struct alarm* alarm) {
    struct alarm* triggered_alarm = calloc(1, sizeof(struct alarm));

    triggered_alarm->head = alarm->head;
    triggered_alarm->listener = alarm->listener;
    triggered_alarm->when = alarm->when;

    return triggered_alarm;
}

static void trigger_alarm_locked(void) {
    uint64_t now = get_sys_time_ms();

    struct list_head* pos;
    struct list_head* next_pos;

    list_for_each_safe(pos, next_pos, &alarm_list) {
        struct alarm* alarm = list_entry(pos, struct alarm, head);

        /*
         * Do not fire alarms in the future
         */
        if (alarm->when > now)
            break;

        if (!list_empty(&trigger_list)) {
            struct list_head* pos;
            struct list_head* next_pos;
            list_for_each_safe(pos, next_pos, &trigger_list) {
                struct alarm* a =
                        list_entry(pos, struct alarm, head);

                list_del(&a->head);
                free(a);
            }
        }
        struct alarm* triggered_alarm = create_triggered_alarm(alarm);

        list_add_tail(&triggered_alarm->head, &trigger_list);

        list_del(&alarm->head);
        free(alarm);
    }

    if (get_list_size_locked(&alarm_list) > 0) {
        struct alarm* alarm = list_first_entry(&alarm_list, struct alarm, head);
        set_locked(alarm);
    }
}

static void thread_loop(struct pthread_wrapper* pthread, void *param) {

    for (;;) {
        pthread_testcancel();

        wait_for_alarm();

        pthread_testcancel();

        pthread_mutex_lock(&alarms_lock);

        trigger_alarm_locked();

        struct list_head* pos;
        list_for_each(pos, &trigger_list) {
            struct alarm* alarm = list_entry(pos, struct alarm, head);

            if (alarm->listener) {
                pthread_mutex_unlock(&alarms_lock);
                alarm->listener();
                pthread_mutex_lock(&alarms_lock);
            }
        }

        pthread_mutex_unlock(&alarms_lock);
    }

    pthread_exit(NULL);
}

static void set(uint64_t when, alarm_listener_t listener) {
    set_repeat(when, 0, listener);
}

static void cancel(alarm_listener_t listener) {
    pthread_mutex_lock(&alarms_lock);

    remove_alarm_locked(listener);

    pthread_mutex_unlock(&alarms_lock);
}

static int start(void) {
    pthread_mutex_lock(&start_lock);

    if (start_count++ == 0)
        thread->start(thread, NULL);

    pthread_mutex_unlock(&start_lock);

    return 0;
}

static int is_start(void) {
    pthread_mutex_lock(&start_lock);
    int started = thread->is_running(thread);
    pthread_mutex_unlock(&start_lock);

    return started;
}

static int stop(void) {
    pthread_mutex_lock(&start_lock);

    if (--start_count == 0) {
        /*
         * For wakeup ANDROID_ALARM_WAIT
         */
        set(get_sys_time_ms() + 100, dummy_listener);

        thread->stop(thread);
    }

    pthread_mutex_unlock(&start_lock);

    return 0;
}

static int init(void) {
    pthread_mutex_lock(&init_lock);

    if (init_count++ == 0) {
        fd = open("/dev/alarm", O_RDWR);
        if (fd < 0) {
            LOGE("Failed to open /dev/alarm: %s\n", strerror(errno));
            goto error;
        }

        thread = _new(struct thread, thread);
        thread->runnable.run = thread_loop;
    }

    pthread_mutex_unlock(&init_lock);

    return 0;

error:
    init_count = 0;
    pthread_mutex_unlock(&init_lock);
    return -1;
}

static int deinit(void) {
    pthread_mutex_lock(&init_lock);

    if (--init_count == 0) {
        if (is_start())
            stop();

        if (fd > 0) {
            close(fd);
            fd = -1;
        }
        _delete(thread);
    }

    pthread_mutex_unlock(&init_lock);

    return 0;
}

static struct alarm_manager this = {
        .init = init,
        .deinit = deinit,
        .start = start,
        .stop = stop,
        .is_start = is_start,
        .set = set,
        .cancel = cancel,
        .get_sys_time_ms = get_sys_time_ms,
};

struct alarm_manager* get_alarm_manager(void) {
    return &this;
}
