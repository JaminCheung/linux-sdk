/*************************************************************************
	> Filename: main.c
	>   Author: Wang Qiuwei
	>    Email: qiuwei.wang@ingenic.com / panddio@163.com
	> Datatime: Thu 15 Dec 2016 04:05:47 PM CST
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#include <utils/log.h>
#include <utils/assert.h>
#include <libqrcode_api.h>

#define LOG_TAG "test_pwm"

/*
 * Variables
 */
const unsigned int duty[] = {
     0,  1,  2,  3,  4,  5,  6,  7,
     9, 10, 11, 13, 15, 17, 19, 21,
    23, 25, 27, 29, 31, 33, 35, 38,
    41, 44, 47, 50, 53, 56, 59, 62,
    66, 70, 74, 79, 84, 89, 94, 100,
};

/*
 * Functions
 */
void print_usage() {
    printf("Usage:\n \
        test_pwm [OPTIONS]\n \
            a pwm test demo\n\n \
        OPTIONS:\n \
            -c   ----> select PWM channel\n \
            -f   ----> set PWM frequency\n \
            -n   ----> set cycle times\n \
            -h   ----> get help message\n\n \
        eg:\n \
        test_pwm -c 2   ----> select PWM channel 2\n \
        test_pwm -n 20  ----> set cycle_times = 20\r\n");
}

static void breathing_lamp(struct pwm_manager *pwm, enum pwm id, int cycle_times) {
    int i;

    while(cycle_times--) {
        for(i = 0; i < sizeof(duty)/sizeof(unsigned int); i++) {
            pwm->setup_duty(id, duty[i]);
            usleep(50000);
        }

        for(i = sizeof(duty)/sizeof(unsigned int) - 1; i >= 0; i--) {
            pwm->setup_duty(id, duty[i]);
            usleep(50000);
        }
    }
}

int main(int argc, char *argv[])
{
    int oc;
    int cycle_times = 10;
    int pwm_id = 0;
    unsigned int freq = 30000;
    struct pwm_manager *pwm;

    while(1) {
        oc = getopt(argc, argv, "hc:f:n:");
        if(oc == -1)
            break;

        switch(oc) {
        case 'c':
            pwm_id = atoi(optarg);
            break;

        case 'f':
            freq = atoi(optarg);
            break;

        case 'n':
            cycle_times = atoi(optarg);
            break;

        case 'h':
        default:
            print_usage();
            return 1;
        }
    }

    /* 获取操作PWM的句柄 */
    pwm = get_pwm_manager();

    pwm->init(pwm_id);
    pwm->setup_freq(pwm_id, freq);

    /* 模拟呼吸灯效果 */
    breathing_lamp(pwm, pwm_id, cycle_times);

    pwm->deinit(pwm_id);
    return 0;
}
