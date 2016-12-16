/*
 *  Copyright (C) 2016, Wang Qiuwei <qiuwei.wang@ingenic.com, panddio@163.com>
 *
 *  Ingenic QRcode SDK Project
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
#include <pwm/pwm_manager.h>


/*
 * Macro
 */
#define LOG_TAG   "pwm"

/*
 * Variables
 */
struct pwm_dev {
    int freq_fd;
    int duty_fd;
    int ctrl_fd;
    int is_init;
    unsigned int max;
};

static struct pwm_dev pwm_dev[PWM_DEVICE_MAX];

/*
 * Functions
 */
static int pwm_init(enum pwm id) {
    int fd;
    int ret;
    char temp[12] = "";
    char node[64] = "";

    assert_die_if(id >= PWM_DEVICE_MAX, "PWM%d is invalid!\n", id);
    if (pwm_dev[id].is_init == 1)
        return 0;

    sprintf(node, "/sys/class/jz-pwm/pwm%d/max_dutyratio", id);

    fd = open(node, O_RDONLY);
    assert_die_if(fd < 0, "Open %s failed: %s\n", node, strerror(errno));

    ret = read(fd, temp, sizeof(temp));
    assert_die_if(ret <= 0, "Read %s error: %s\n", node, strerror(errno));

    pwm_dev[id].max = atoi(temp);
    close(fd);

    bzero(&node, sizeof(node));
    sprintf(node, "/sys/class/jz-pwm/pwm%d/dutyratio", id);
    fd = open(node, O_WRONLY);
    assert_die_if(fd < 0, "Open %s failed: %s\n", node, strerror(errno));
    pwm_dev[id].duty_fd = fd;

    bzero(&node, sizeof(node));
    sprintf(node, "/sys/class/jz-pwm/pwm%d/period", id);
    fd = open(node, O_WRONLY);
    assert_die_if(fd < 0, "Open %s failed: %s\n", node, strerror(errno));
    pwm_dev[id].freq_fd = fd;

    bzero(&node, sizeof(node));
    sprintf(node, "/sys/class/jz-pwm/pwm%d/enable", id);
    fd = open(node, O_WRONLY);
    assert_die_if(fd < 0, "Open %s failed: %s\n", node, strerror(errno));
    pwm_dev[id].ctrl_fd = fd;

    pwm_dev[id].is_init = 1;
    return 0;
}

static void pwm_deinit(enum pwm id) {

    assert_die_if(id >= PWM_DEVICE_MAX, "PWM%d is invalid!\n", id);
    if (pwm_dev[id].is_init == 0)
        return;

    close(pwm_dev[id].freq_fd);
    close(pwm_dev[id].duty_fd);
    close(pwm_dev[id].ctrl_fd);
    pwm_dev[id].is_init = 0;
    pwm_dev[id].freq_fd = -1;
    pwm_dev[id].duty_fd = -1;
    pwm_dev[id].ctrl_fd = -1;
}

static int pwm_setup_freq(enum pwm id, unsigned int freq) {
    int size;
    char temp[12] = "";

    assert_die_if(id >= PWM_DEVICE_MAX, "PWM%d is invalid!\n", id);
    if (pwm_dev[id].is_init == 0)
        return -1;

    size = snprintf(temp, sizeof(temp), "%d", freq);
    if (write(pwm_dev[id].freq_fd, temp, size) < 0) {
        LOGE("Failed to setup PWM%d freq: %s\n", id, strerror(errno));
        return -1;
    }

    return 0;
}

static int pwm_setup_duty(enum pwm id, unsigned int duty) {
    float val = 0.0;
    int size;
    char temp[12] = "";

    assert_die_if(id >= PWM_DEVICE_MAX, "PWM%d is invalid!\n", id);
    if (pwm_dev[id].is_init == 0)
        return -1;

    if (duty > 100)
        duty = 100;
    if (duty < 0)
        duty = 0;

    val = duty / 100.0 * pwm_dev[id].max;
    size = snprintf(temp, sizeof(temp), "%d", (int)val);
    if (write(pwm_dev[id].duty_fd, temp, size) < 0) {
        LOGE("Failed to setup PWM%d duty: %s\n", id, strerror(errno));
        return -1;
    }

    return 0;
}

static int pwm_setup_state(enum pwm id, enum pwm_state state) {
    int size;
    char temp[6] = "";

    assert_die_if(id >= PWM_DEVICE_MAX, "PWM%d is invalid!\n", id);
    if (pwm_dev[id].is_init == 0)
        return -1;

    size = snprintf(temp, sizeof(temp), "%d", !!state);
    if (write(pwm_dev[id].ctrl_fd, temp, size) < 0) {
        LOGE("Failed to setup PWM%d freq: %s\n", id, strerror(errno));
        return -1;
    }

    return 0;
}

struct pwm_manager pwm_manager = {
    .init        = pwm_init,
    .deinit      = pwm_deinit,
    .setup_freq  = pwm_setup_freq,
    .setup_duty  = pwm_setup_duty,
    .setup_state = pwm_setup_state,
};

struct pwm_manager *get_pwm_manager(void) {
    return &pwm_manager;
}
