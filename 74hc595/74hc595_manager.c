/*
 *  Copyright (C) 2016, Wang Qiuwei <qiuwei.wang@ingenic.com, panddio@163.com>
 *
 *  Ingenic SDK Project
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

#include <types.h>
#include <utils/log.h>
#include <utils/assert.h>
#include <74hc595/74hc595_manager.h>

/*
 * Macros
 */
#define LOG_TAG  "74hc595"

/*
 * ioctl commands
 */
#define SN74HC595_IOC_MAGIC         'J'
#define SN74HC595_IOC_WRITE         _IOW(SN74HC595_IOC_MAGIC, 1, int)   /* set output data */
#define SN74HC595_IOC_READ          _IOR(SN74HC595_IOC_MAGIC, 2, int)   /* get last output data */
#define SN74HC595_IOC_CLEAR         _IOW(SN74HC595_IOC_MAGIC, 3, int)   /* clear shift register */
#define SN74HC595_IOC_GET_OUTBITS   _IOR(SN74HC595_IOC_MAGIC, 4, int)   /* get output bits length */


struct sn74hc595_dev {
    int fd;
    bool is_init;
    pthread_mutex_t lock;
};

static struct sn74hc595_dev sndev[SN74HC595_DEVICE_NUM];

/*
 * Functions
 */
static uint32_t sn74hc595_get_outbits(enum sn74hc595 id, uint32_t *out_bits) {
    int32_t retval;

    assert_die_if(id >= SN74HC595_DEVICE_NUM, "SN74HC595 dev%d is invalid!\n", id);
    pthread_mutex_lock(&sndev[id].lock);
    if (sndev[id].is_init == false) {
        LOGE("Get outbits failed: 74HC595 is not initialized\n");
        goto err_get_outbits;
    }

    retval = ioctl(sndev[id].fd, SN74HC595_IOC_GET_OUTBITS, out_bits);
    if (retval < 0) {
        LOGE("Failed to get outbits: %s\n", strerror(errno));
        goto err_get_outbits;
    }
    pthread_mutex_unlock(&sndev[id].lock);
    return retval;

err_get_outbits:
    pthread_mutex_unlock(&sndev[id].lock);
    return -1;
}

static int32_t sn74hc595_clear(enum sn74hc595 id) {
    int32_t retval;

    assert_die_if(id >= SN74HC595_DEVICE_NUM, "SN74HC595 dev%d is invalid!\n", id);
    pthread_mutex_lock(&sndev[id].lock);
    if (sndev[id].is_init == false) {
        LOGE("Clear failed: 74HC595 is not initialized\n");
        goto err_clear;
    }

    retval = ioctl(sndev[id].fd, SN74HC595_IOC_CLEAR, NULL);
    if (retval < 0) {
        LOGE("Failed to clear shift register: %s\n", strerror(errno));
        goto err_clear;
    }

    pthread_mutex_unlock(&sndev[id].lock);
    return retval;

err_clear:
    pthread_mutex_unlock(&sndev[id].lock);
    return -1;
}

static int32_t sn74hc595_read(enum sn74hc595 id, void *data, uint32_t out_bits) {
    int32_t retval;

    assert_die_if(id >= SN74HC595_DEVICE_NUM, "SN74HC595 dev%d is invalid!\n", id);
    pthread_mutex_lock(&sndev[id].lock);
    if (sndev[id].is_init == false) {
        LOGE("Read failed: 74HC595 is not initialized\n");
        goto err_read;
    }

    retval = ioctl(sndev[id].fd, SN74HC595_IOC_READ, data);
    if (retval != out_bits) {
        LOGE("Failed to read last data: %s\n", strerror(errno));
        goto err_read;
    }

    pthread_mutex_unlock(&sndev[id].lock);
    return retval;

err_read:
    pthread_mutex_unlock(&sndev[id].lock);
    return -1;
}

static int32_t sn74hc595_write(enum sn74hc595 id, void *data, uint32_t out_bits) {
    int32_t retval;

    assert_die_if(id >= SN74HC595_DEVICE_NUM, "SN74HC595 dev%d is invalid!\n", id);
    pthread_mutex_lock(&sndev[id].lock);
    if (sndev[id].is_init == false) {
        LOGE("Write failed: 74HC595 is not initialized\n");
        goto err_write;
    }

    retval = ioctl(sndev[id].fd, SN74HC595_IOC_WRITE, data);
    if (retval != out_bits) {
        LOGE("Failed to write 74HC595: %s\n", strerror(errno));
        goto err_write;
    }

    pthread_mutex_unlock(&sndev[id].lock);
    return retval;

err_write:
    pthread_mutex_unlock(&sndev[id].lock);
    return -1;
}

static int32_t sn74hc595_init(enum sn74hc595 id) {
    char devname[32] = "";

    assert_die_if(id >= SN74HC595_DEVICE_NUM, "SN74HC595 dev%d is invalid!\n", id);
    if (sndev[id].is_init) {
        LOGI("WARNING: 74HC595 has been initialized\n");
        return 0;
    }

    snprintf(devname, sizeof(devname), "/dev/sn74hc595_%d", id);
    sndev[id].fd = open(devname, O_RDWR);
    assert_die_if(sndev[id].fd < 0, "Failed to open 74HC595 dev: %s\n", strerror(errno));

    sndev[id].is_init = true;
    pthread_mutex_init(&sndev[id].lock, NULL);
    return 0;
}

static void sn74hc595_deinit(enum sn74hc595 id) {
    assert_die_if(id >= SN74HC595_DEVICE_NUM, "SN74HC595 dev%d is invalid!\n", id);
    pthread_mutex_lock(&sndev[id].lock);
    close(sndev[id].fd);
    sndev[id].fd = -1;
    sndev[id].is_init = false;
    pthread_mutex_unlock(&sndev[id].lock);
}

struct sn74hc595_manager sn74hc595_manager = {
    .init   = sn74hc595_init,
    .deinit = sn74hc595_deinit,
    .write  = sn74hc595_write,
    .read   = sn74hc595_read,
    .clear  = sn74hc595_clear,
    .get_outbits = sn74hc595_get_outbits,
};

struct sn74hc595_manager *get_sn74hc595_manager(void) {
    return &sn74hc595_manager;
}
