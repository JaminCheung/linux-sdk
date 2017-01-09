/*
 *  Copyright (C) 2016, Zhang YanMing <yanmin.zhang@ingenic.com, jamincheung@126.com>
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include <utils/log.h>
#include <libqrcode_api.h>

#define LOG_TAG "security"

#define AES_IOCTL_MAGIC 'A'
#define AES_LOAD_KEY    _IOW(AES_IOCTL_MAGIC, 0x01, struct aes_key)
#define AES_DO_CRYPT    _IOWR(AES_IOCTL_MAGIC, 0x03, struct aes_data)

#define DRV_NAME    "/dev/jz-aes"

static int32_t security_simple_aes_load_key(struct aes_key* aes_key) {
    int error = 0;

    int32_t fd = open(DRV_NAME, O_RDWR);
    if (fd < 0) {
        LOGE("Failed to open %s: %s\n", DRV_NAME, strerror(errno));
        return -1;
    }

    error = ioctl(fd, AES_LOAD_KEY, aes_key);
    if (error < 0) {
        LOGE("Failed to ioctl AES_LOAD_KEY\n");
        return -1;
    }

    close(fd);

    return 0;
}

static int32_t security_simple_aes_crypt(struct aes_data* aes_data) {
    int error = 0;

    int32_t fd = open(DRV_NAME, O_RDWR);
    if (fd < 0) {
        LOGE("Failed to open %s: %s\n", DRV_NAME, strerror(errno));
        return -1;
    }

    error = ioctl(fd, AES_DO_CRYPT, aes_data);
    if (error < 0) {
        LOGE("Failed to ioctl AES_DO_CRYPT\n");
        return -1;
    }

    close(fd);

    return 0;
}

struct security_manager security_manager = {
    .simple_aes_load_key = security_simple_aes_load_key,
    .simple_aes_crypt = security_simple_aes_crypt,
};

struct security_manager* get_security_manager(void) {
    return & security_manager;
}
