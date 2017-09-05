/*
 *  Copyright (C) 2016, Wang Qiuwei <qiuwei.wang@ingenic.com, panddio@163.com>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

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
    bool is_init;
    uint32_t max;
};

static struct pwm_dev pwm_dev[PWM_CHANNEL_MAX];

/*
 * Functions
 */
static int32_t pwm_init(enum pwm id, enum pwm_active level) {
    int fd;
    int ret;
    int size;
    char temp[12] = "";
    char node[64] = "";

    assert_die_if(id >= PWM_CHANNEL_MAX, "PWM%d is invalid!\n", id);
    if (pwm_dev[id].is_init == true)
        return 0;

    /* set PWM active level */
    snprintf(node, sizeof(node), "/sys/class/jz-pwm/pwm%d/active_level", id);
    fd = open(node, O_WRONLY);
    assert_die_if(fd < 0, "Open %s failed: %s\n", node, strerror(errno));
    size = sprintf(temp, "%d", level);
    if (write(fd, temp, size) < 0) {
        LOGE("Failed to set PWM%d active level: %s\n", \
             id, strerror(errno));
        return -1;
    }
    close(fd);

    /* read the max dutyratio */
    snprintf(node, sizeof(node), "/sys/class/jz-pwm/pwm%d/max_dutyratio", id);
    fd = open(node, O_RDONLY);
    assert_die_if(fd < 0, "Open %s failed: %s\n", node, strerror(errno));

    ret = read(fd, temp, sizeof(temp));
    assert_die_if(ret <= 0, "Read %s error: %s\n", node, strerror(errno));
    pwm_dev[id].max = atoi(temp);
    close(fd);

    snprintf(node, sizeof(node), "/sys/class/jz-pwm/pwm%d/dutyratio", id);
    fd = open(node, O_WRONLY);
    assert_die_if(fd < 0, "Open %s failed: %s\n", node, strerror(errno));
    pwm_dev[id].duty_fd = fd;

    snprintf(node, sizeof(node), "/sys/class/jz-pwm/pwm%d/period", id);
    fd = open(node, O_WRONLY);
    assert_die_if(fd < 0, "Open %s failed: %s\n", node, strerror(errno));
    pwm_dev[id].freq_fd = fd;

    snprintf(node, sizeof(node), "/sys/class/jz-pwm/pwm%d/enable", id);
    fd = open(node, O_WRONLY);
    assert_die_if(fd < 0, "Open %s failed: %s\n", node, strerror(errno));
    pwm_dev[id].ctrl_fd = fd;

    pwm_dev[id].is_init = true;
    return 0;
}

static int32_t pwm_setup_freq(enum pwm id, uint32_t freq) {
    int size;
    char temp[12] = "";

    assert_die_if(id >= PWM_CHANNEL_MAX, "PWM%d is invalid!\n", id);
    if (pwm_dev[id].is_init == false) {
        LOGE("The PWM%d is not initialized!\n", id);
        return -1;
    }

    size = snprintf(temp, sizeof(temp), "%d", freq);
    if (write(pwm_dev[id].freq_fd, temp, size) < 0) {
        LOGE("Failed to set PWM%d freq: %s\n", id, strerror(errno));
        return -1;
    }

    return 0;
}

static int32_t pwm_setup_duty(enum pwm id, uint32_t duty) {
    float val = 0.0;
    int size;
    char temp[12] = "";

    assert_die_if(id >= PWM_CHANNEL_MAX, "PWM%d is invalid!\n", id);
    if (pwm_dev[id].is_init == false) {
        LOGE("The PWM%d is not initialized!\n", id);
        return -1;
    }

    if (duty > 100)
        duty = 100;
    if (duty < 0)
        duty = 0;

    val = duty / 100.0 * pwm_dev[id].max;
    size = snprintf(temp, sizeof(temp), "%d", (int)val);
    if (write(pwm_dev[id].duty_fd, temp, size) < 0) {
        LOGE("Failed to set PWM%d duty: %s\n", id, strerror(errno));
        return -1;
    }

    return 0;
}

static int32_t pwm_setup_state(enum pwm id, enum pwm_state state) {
    int size;
    char temp[6] = "";

    assert_die_if(id >= PWM_CHANNEL_MAX, "PWM%d is invalid!\n", id);
    if (pwm_dev[id].is_init == false) {
        LOGE("The PWM%d is not initialized!\n", id);
        return -1;
    }

    size = snprintf(temp, sizeof(temp), "%d", !!state);
    if (write(pwm_dev[id].ctrl_fd, temp, size) < 0) {
        LOGE("Failed to set PWM%d state: %s\n", id, strerror(errno));
        return -1;
    }

    return 0;
}

static void pwm_deinit(enum pwm id) {
    if (!pwm_setup_state(id, PWM_DISABLE)) {
        close(pwm_dev[id].freq_fd);
        close(pwm_dev[id].duty_fd);
        close(pwm_dev[id].ctrl_fd);

        pwm_dev[id].is_init = false;
        pwm_dev[id].freq_fd = -1;
        pwm_dev[id].duty_fd = -1;
        pwm_dev[id].ctrl_fd = -1;
    }
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
