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

#include <string.h>
#include <sys/time.h>

#include <utils/log.h>
#include <utils/common.h>
#include <utils/runnable/default_runnable.h>
#include <utils/signal_handler.h>
#include "config.h"

#define LOG_TAG "MAFactory"

#define PASS    0
#define FAIL    -1
#define NO_FINGER 0

enum {
    SPI_TEST = 1,
    CALIBRATE_TEST,
    DEADPIX_TEST,
    INTERRUPT_TEST,
    PRESS_TEST,
    ENROLL_TEST,
    MATCH_TEST,
    UNTEST = -100
};

static struct signal_handler* signal_handler;
static struct default_runnable* runner;

static int has_calibrate;
static int ended;
static int test_success;

const char* test_items_str[] = {
        [SPI_TEST]       = "  1. SPI Test",
        [CALIBRATE_TEST] = "  2. Calibrate Test",
        [DEADPIX_TEST]   = "  3. DeadPixel Test",
        [INTERRUPT_TEST] = "  4. Interrupt Test",
        [PRESS_TEST]     = "  5. Press Test",
        [ENROLL_TEST]    = "  6. Enroll Test",
        [MATCH_TEST]     = "  7. Match Test",
};

static int default_test_items[] =  {SPI_TEST, CALIBRATE_TEST, DEADPIX_TEST,
        INTERRUPT_TEST, PRESS_TEST, ENROLL_TEST, MATCH_TEST};
static int test_items[ARRAY_SIZE(default_test_items)];
static int test_items_count;

static void print_test_items(void) {
    fprintf(stderr, "\n============== Test Items ==============\n");
    for (int i = 0; i < ARRAY_SIZE(test_items); i++) {
        int id = test_items[i];
        if (id != 0)
            fprintf(stderr, "%s\n", test_items_str[id]);
    }
    fprintf(stderr, "=======================================\n");
}

////////////////////////////////////////////////////////////////////////////////
int nativeSet2FactoryMode(void) {
    return 0;
}

int nativeSet2NormalMode(void) {
    return 0;
}

int nativeSPICommunicate(void) {
    return 0;
}

int nativeBadImage(void) {
    return 0;
}

int nativeInterrupt(void) {
    return 0;
}

int nativePress(void) {
    return 0;
}

int nativeEnroll(void) {
    return 0;
}

int nativeAuthenticate(void) {
    return 0;
}

int nativeRemove(void) {
    return 0;
}

const char* nativeGetVendor(void) {
    return "120345";
}

int nativeCalibrate(void) {
    return 0;
}
////////////////////////////////////////////////////////////////////////////////


static void finish(void) {
    msleep(500);

    exit(0);
}

static int64_t get_current_time_ms(void) {
    struct timeval tv;

    gettimeofday(&tv, NULL);

    return ((uint64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000) + 0.5;
}

static void handle_signal(int signal) {
    LOGI("signal:%d\n", signal);
    LOGW("Please wait test done.\n");
    //TODO
    exit(1);
}

static int get_enroll_steps(int times) {
    return (100 % times == 0 ? 100 / times : 100 / times + 1);
}

static int test_spi(void) {
    int ret = -1;
    for (int i = 0; i < 10; i++) {
        LOGI("test spi start i=%d\n", i);
        ret = nativeSPICommunicate();
        LOGI("test spi end. ret=%d\n", ret);

        if (ret == PASS)
            break;
    }
    ended = ret;

    return ended == FAIL;
}

static int test_calibrate(void) {
    int ret;

    LOGI("test callibrate start\n");
    ret = nativeCalibrate();
    LOGI("test callibrate end. ret=%d\n", ret);

    ended = ret;

    return ended == FAIL;
}

static int test_deadpix(void) {
    int ret;

    LOGI("test deadpix start\n");
    ret = nativeBadImage();
    LOGI("test deadpix end. ret=%d\n", ret);

    ended = ret;

    return ended == FAIL;
}

static int test_interrupt(void) {
    int ret;

    for (int i = 0; i < 10; i++) {
        LOGI("test interrupt start i=%d\n", i);
        ret = nativeInterrupt();
        LOGI("test interrupt end. ret=%d\n", ret);
    }

    ended = ret;

    return ended == FAIL;
}

static int test_press(void) {
    int64_t origin = get_current_time_ms();

    //wait finger down
    while (1) {
        LOGI("test press start\n");

        int64_t energy = nativePress();
        LOGI("test press end. ret=%lld\n", energy);
        if (energy > NO_FINGER)
            break;
        else if (get_current_time_ms() - origin > PRESS_TIMEOUT) {
            ended = 1;
            return -1;
        }
    }

    //wait finger up
    while (1) {
        LOGI("test press start\n");
        int64_t energy = nativePress();
        LOGI("test press end. ret=%lld\n", energy);
        if (energy == NO_FINGER)
            break;
        else if (get_current_time_ms() - origin > PRESS_TIMEOUT) {
            ended = 1;
            return -1;
        }
    }

    return 0;
}

static int test_enroll(void) {
    int64_t energy = 0;
    int ret = 0;
    int percent = 0;
    int error = -1;

    while (1) {
        LOGI("test enroll-press start\n");
        energy = nativePress();
        LOGI("test enroll-press end. ret=%d\n", ret);

        if (energy > NO_FINGER) {
            percent += get_enroll_steps(ENROLL_TIMES);

            percent = percent > 100 ? 100 : percent;
            LOGI("percent=%d\n", percent);

            if (percent >= 100)
                error = 0;

            // wait finger up
            while (1) {
                LOGI("test enroll-press start\n");
                energy = nativePress();
                LOGI("test enroll-press end\n");

                if (energy == NO_FINGER)
                    break;
            }

            LOGI("test enroll start\n");
            energy = nativeEnroll();
            LOGI("test enroll end. ret=%d\n", ret);
            if (energy == NO_FINGER)
                break;

            ret = nativeEnroll();
            if (percent >= 100)
                break;
        }
    }

    return error;
}

static int test_match(void) {
    int energy = 0;
    int ret = 0;
    int identify_count = 0;
    int error = -1;

    while (1) {
        LOGI("test match-press start\n");
        energy = nativePress();
        LOGI("test match-press end. ret=%d\n", ret);

        if (energy > NO_FINGER) {
            //authenticate
            for (int i = 0; i < 3; i++) {
                LOGI("test authenticate start\n");
                ret = nativeAuthenticate();
                LOGI("test authenticate ret=%d\n", ret);
                if (ret == PASS)
                    break;

                if (i < 2) {
                    LOGI("test match-press start\n");
                    energy = nativePress();
                    LOGI("test match-press end. ret=%d\n", energy);
                }
            }

            if (ret == PASS) {
                error = 0;
                break;
            } else {
                identify_count++;
                if (identify_count < MATCH_TIMES) {
                    LOGI("identify_count=%d\n", identify_count);
                } else {
                    error = -1;
                    break;
                }
            }

            while (1) {
                LOGI("test match-press start\n");
                energy = nativePress();
                LOGI("test match-press end. ret=%d\n", energy);
                if (energy == NO_FINGER)
                    break;
            }
        }
    }


    return error;
}

static int set_factory_mode(void) {
    int set = nativeSet2FactoryMode();
    LOGI("set factory=%d\n", set);
    return set == PASS;
}

static void set_normal_mode(void) {
    int set = nativeSet2NormalMode();
    LOGI("set normal=%d\n", set);
}

static void work_thread(struct pthread_wrapper* thread, void* param) {
    test_success = 1;

    for (;;) {
        ended = !set_factory_mode();
        int i;
        int ret = 0;

        for (i = 0; i < ARRAY_SIZE(test_items); i++) {
            if (test_items[i] == 0)
                continue;

            if (ended)
                break;

            int id = test_items[i];
            switch (id) {
            case SPI_TEST:
                ret = test_spi();
                break;

            case CALIBRATE_TEST:
                ret = test_calibrate();
                break;

            case DEADPIX_TEST:
                ret = test_deadpix();
                break;

            case INTERRUPT_TEST:
                ret = test_interrupt();
                break;

            case PRESS_TEST:
                ret = test_press();
                break;

            case ENROLL_TEST:
                ret = test_enroll();
                break;

            case MATCH_TEST:
                ret = test_match();
                break;
            }

            if (ret != 0)
                break;
        }

        nativeRemove();
        set_normal_mode();

        if (i == test_items_count) {
            test_success = 1;
        } else {
            test_success = 0;
        }

        break;
    }
}

static void init_test(void) {
    set_factory_mode();
    const char* vendor = nativeGetVendor();
    set_normal_mode();

    if (strstr(vendor, "120"))
        has_calibrate = 1;
    else
        has_calibrate = 0;

    for (int i = 0; i < ARRAY_SIZE(default_test_items); i++) {
        int id = default_test_items[i];
        if (id == CALIBRATE_TEST)
            continue;
        test_items[i] = id;
        test_items_count++;
    }

    runner = _new(struct default_runnable, default_runnable);
    runner->runnable.run = work_thread;

    signal_handler = _new(struct signal_handler, signal_handler);
    signal_handler->set_signal_handler(signal_handler, SIGINT, handle_signal);
    signal_handler->set_signal_handler(signal_handler, SIGQUIT, handle_signal);
    signal_handler->set_signal_handler(signal_handler, SIGTERM, handle_signal);
}

static void do_work(void) {
    char c = 0;

    print_test_items();

restart:
    fflush(stdin);
    printf("Type \'s\' to start test\n");
    scanf("%c", &c);

    if (c != 's')
        goto restart;

    LOGI("Start testing...\n");

    runner->start(runner, NULL);

    runner->wait(runner);

    if (!test_success) {
        LOGI("Test failure, restart.\n");
        goto restart;
    } else {
        LOGI("Test success.\n");
        finish();
    }
}

int main(int argc, char* argv[]) {
    init_test();

    do_work();

    while (1)
        sleep(1000);
}
