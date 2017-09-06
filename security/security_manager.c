/*
 *  Copyright (C) 2016, Zhang YanMing <yanmin.zhang@ingenic.com, jamincheung@126.com>
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
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include <utils/log.h>
#include <security/security_manager.h>

#define LOG_TAG "security"

#define AES_IOCTL_MAGIC 'A'
#define AES_LOAD_KEY    _IOW(AES_IOCTL_MAGIC, 0x01, struct aes_key)
#define AES_DO_CRYPT    _IOWR(AES_IOCTL_MAGIC, 0x03, struct aes_data)

#define DRV_NAME    "/dev/jz-aes"

static int32_t fd = -1;
static uint32_t init_count;

static pthread_mutex_t init_lock = PTHREAD_MUTEX_INITIALIZER;

static int security_init() {
    pthread_mutex_lock(&init_lock);

    if (init_count++ == 0) {
        fd = open(DRV_NAME, O_RDWR);
        if (fd < 0) {
            LOGE("Failed to open %s: %s\n", DRV_NAME, strerror(errno));
            goto error;
        }
    }

    pthread_mutex_unlock(&init_lock);

    return 0;

error:
    init_count = 0;
    pthread_mutex_unlock(&init_lock);
    return -1;
}

static void security_deinit() {
    pthread_mutex_lock(&init_lock);

    if (--init_count == 0) {
        if (fd > 0)
            close(fd);

        fd = -1;
    }

    pthread_mutex_unlock(&init_lock);
}

static int32_t security_simple_aes_load_key(struct aes_key* aes_key) {
    int error = 0;

    if (fd < 0)
        return -1;

    error = ioctl(fd, AES_LOAD_KEY, aes_key);
    if (error < 0) {
        LOGE("Failed to ioctl AES_LOAD_KEY: %d: %s\n", error, strerror(errno));
        return -1;
    }

    return 0;
}

static int32_t security_simple_aes_crypt(struct aes_data* aes_data) {
    int error = 0;

    if (fd < 0)
        return -1;

    error = ioctl(fd, AES_DO_CRYPT, aes_data);
    if (error < 0) {
        LOGE("Failed to ioctl AES_DO_CRYPT\n");
        return -1;
    }

    return 0;
}

struct security_manager security_manager = {
    .init   = security_init,
    .deinit = security_deinit,
    .simple_aes_load_key = security_simple_aes_load_key,
    .simple_aes_crypt = security_simple_aes_crypt,
};

struct security_manager* get_security_manager(void) {
    return & security_manager;
}
