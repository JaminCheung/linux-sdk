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
#include <limits.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/fs.h>

#include <types.h>
#include <utils/log.h>
#include <utils/assert.h>
#include <lib/i2c/libsmbus.h>
#include <i2c/i2c_manager.h>

/*
 * Macros
 */
#define CHECK_I2C_FUNC(funcs, lable)                \
    do {                                            \
            assert_die_if(0 == (funcs & lable),     \
            #lable " i2c function is required.");   \
    } while (0);


/*
 * Macros
 */
#define LOG_TAG  "i2c"

/*
 * Variables
 */
struct i2c_bus {
    int fd;
    int dev_count;
    bool is_init;
    pthread_mutex_t lock;
};

static struct i2c_bus i2c_bus[I2C_BUS_MAX];
static pthread_mutex_t init_lock = PTHREAD_MUTEX_INITIALIZER;

/*
 * Functions
 */
static int32_t i2c_write_1b(int fd, uint8_t *buf) {
    int retval = 0;

    retval = i2c_smbus_write_byte(fd, buf[0]);
    if (retval < 0)
        LOGE("Failed to write 1 byte: %s\n", strerror(errno));

    usleep(I2C_ACCESS_DELAY_US);

    return retval;
}

static int32_t i2c_write_2b(int fd, uint8_t *buf) {
    int retval = 0;

    retval = i2c_smbus_write_byte_data(fd, buf[0], buf[1]);
    if (retval)
        LOGE("Failed to write 2 bytes: %s\n", strerror(errno));

    usleep(I2C_ACCESS_DELAY_US);

    return retval;
}

#if (I2C_DEV_ADDR_LENGTH == 16)
static int32_t i2c_write_3b(int fd, uint8_t *buf) {
    int retval = 0;

    retval = i2c_smbus_write_word_data(fd, buf[0], buf[2] << 8 | buf[1]);
    if (retval)
        LOGE("Failed to write 3 bytes: %s\n", strerror(errno));

    usleep(I2C_ACCESS_DELAY_US);

    return retval;
}
#endif

static int32_t i2c_write_byte(int fd, int addr, uint8_t data) {
#if (I2C_DEV_ADDR_LENGTH == 8)
    uint8_t buf[2];

    buf[0] = addr & 0xff;
    buf[1] = data;

    return i2c_write_2b(fd, buf);
#elif (I2C_DEV_ADDR_LENGTH == 16)
    uint8_t buf[3];

    buf[0] = (addr >> 8) & 0xff;
    buf[1] = addr & 0xff;
    buf[2] = data;

    return i2c_write_3b(fd, buf);
#else
#error "Unknown I2C device type"
#endif
}

static int32_t i2c_read_byte(int fd, int addr) {
    int retval = 0;

    ioctl(fd, BLKFLSBUF);

#if (I2C_DEV_ADDR_LENGTH == 8)
    uint8_t buf[1];

    buf[0] = addr & 0xff;

    retval = i2c_write_1b(fd, buf);
    if (retval < 0)
        return retval;

    retval = i2c_smbus_read_byte(fd);

    return retval;
#elif (I2C_DEV_ADDR_LENGTH == 16)
    uint8_t buf[2];

    buf[0] = (addr >> 8) & 0xff;
    buf[1] = addr & 0xff;

    retval = i2c_write_2b(fd, buf);
    if (retval < 0)
        return retval;

    retval = i2c_smbus_read_byte(fd);

    return retval;
#else
#error "Unknown I2C device type"
#endif
}

static int32_t i2c_check_chip_addr(struct i2c_unit *i2c) {
    int i;
    int retval;

    pthread_mutex_lock(&i2c_bus[i2c->id].lock);
    retval = i2c_smbus_set_slave_addr(i2c_bus[i2c->id].fd, i2c->chip_addr, true);
    if (retval < 0) {
        LOGE("Failed to set I2C-%d chip addr: %s\n", i2c->id, strerror(errno));
        goto exit;
    }

    for (i = 0; i < 5; i++) {
        retval = i2c_read_byte(i2c_bus[i2c->id].fd, I2C_CHECK_READ_ADDR + i);
        if (retval >= 0) {
            pthread_mutex_unlock(&i2c_bus[i2c->id].lock);
            return 0;
        }
    }

exit:
    pthread_mutex_unlock(&i2c_bus[i2c->id].lock);
    return -1;
}

static int32_t i2c_write(struct i2c_unit *i2c, uint8_t *buf, int addr, int count) {
    int i;
    int retval;

    assert_die_if(!buf, "Error: buffer pointer cannot be 'NULL'\n");
    assert_die_if(i2c->id >= I2C_BUS_MAX, "I2C-%d is invalid!\n", i2c->id);
    if (i2c_bus[i2c->id].is_init == false) {
        LOGE("Write failed: I2C-%d is not initialized!\n", i2c->id);
        return -1;
    }

    pthread_mutex_lock(&i2c_bus[i2c->id].lock);
    retval = i2c_smbus_set_slave_addr(i2c_bus[i2c->id].fd, i2c->chip_addr, true);
    if (retval < 0) {
        LOGE("Failed to set I2C-%d chip addr: %s\n", i2c->id, strerror(errno));
        goto err_write;
    }

    for (i = 0; i < count; i++) {
        retval = i2c_write_byte(i2c_bus[i2c->id].fd, addr + i, buf[i]);
        if (retval < 0) {
            LOGE("Failed to write I2C-%d: %s\n", i2c->id, strerror(errno));
            goto err_write;
        }
    }

    pthread_mutex_unlock(&i2c_bus[i2c->id].lock);
    return 0;

err_write:
    pthread_mutex_unlock(&i2c_bus[i2c->id].lock);
    return retval;
}

static int32_t i2c_read(struct i2c_unit *i2c, uint8_t *buf, int addr, int count) {
    int i;
    int retval;

    assert_die_if(!buf, "Error: buffer pointer cannot be 'NULL'\n");
    assert_die_if(i2c->id >= I2C_BUS_MAX, "I2C-%d is invalid!\n", i2c->id);
    if (i2c_bus[i2c->id].is_init == false) {
        LOGE("Read failed: I2C-%d is not initialized!\n", i2c->id);
        return -1;
    }

    pthread_mutex_lock(&i2c_bus[i2c->id].lock);
    retval = i2c_smbus_set_slave_addr(i2c_bus[i2c->id].fd, i2c->chip_addr, true);
    if (retval < 0) {
        LOGE("Failed to set I2C-%d chip addr: %s\n", i2c->id, strerror(errno));
        goto err_read;
    }

    for (i = 0; i < count; i++) {
        buf[i] = i2c_read_byte(i2c_bus[i2c->id].fd, addr + i);
        if(buf[i] < 0) {
            LOGE("Failed to read I2C-%d: %s\n", i2c->id, strerror(errno));
            goto err_read;
        }
    }

    pthread_mutex_unlock(&i2c_bus[i2c->id].lock);
    return 0;

err_read:
    pthread_mutex_unlock(&i2c_bus[i2c->id].lock);
    return retval;
}

static int32_t i2c_init(struct i2c_unit *i2c) {
    char filename[32] = "";
    int retval = 0;
    unsigned long funcs;

    assert_die_if(i2c->id >= I2C_BUS_MAX, "I2C-%d is invalid!\n", i2c->id);

    pthread_mutex_lock(&init_lock);
    if (i2c_bus[i2c->id].is_init == false) {
        retval = i2c_smbus_open(i2c->id);
        if(retval < 0) {
            LOGE("Failed to open %s: %s\n", filename, strerror(errno));
            goto err_init;
        }
        i2c_bus[i2c->id].fd = retval;

        retval = i2c_smbus_get_funcs_matrix(i2c_bus[i2c->id].fd, &funcs);
        if(retval < 0) {
            LOGE("Failed to get I2C-%d functions: %s\n", i2c->id, strerror(errno));
            i2c_smbus_close(i2c_bus[i2c->id].fd);
            goto err_init;
        }

        CHECK_I2C_FUNC(funcs, I2C_FUNC_SMBUS_READ_BYTE);
        CHECK_I2C_FUNC(funcs, I2C_FUNC_SMBUS_WRITE_BYTE);
        CHECK_I2C_FUNC(funcs, I2C_FUNC_SMBUS_READ_BYTE_DATA);
        CHECK_I2C_FUNC(funcs, I2C_FUNC_SMBUS_WRITE_BYTE_DATA);
        CHECK_I2C_FUNC(funcs, I2C_FUNC_SMBUS_READ_WORD_DATA);
        CHECK_I2C_FUNC(funcs, I2C_FUNC_SMBUS_WRITE_WORD_DATA);

        pthread_mutex_init(&i2c_bus[i2c->id].lock, NULL);
    }

    retval = i2c_check_chip_addr(i2c);
    if(retval < 0) {
        LOGE("Failed to check I2C-%d chip addr: %0x\n", i2c->id, i2c->chip_addr);
        goto err_init;
    }

    i2c_bus[i2c->id].is_init = true;
    i2c_bus[i2c->id].dev_count++;
    pthread_mutex_unlock(&init_lock);
    return 0;

err_init:
    pthread_mutex_unlock(&init_lock);
    return retval;
}

static void i2c_deinit(struct i2c_unit *i2c) {
    assert_die_if(i2c->id >= I2C_BUS_MAX, "I2C-%d is invalid!\n", i2c->id);

    pthread_mutex_lock(&init_lock);
    if (i2c_bus[i2c->id].dev_count == 1) {
        i2c_smbus_close(i2c_bus[i2c->id].fd);
        i2c_bus[i2c->id].fd = -1;
        i2c_bus[i2c->id].is_init = false;
        pthread_mutex_destroy(&i2c_bus[i2c->id].lock);
    }

    i2c_bus[i2c->id].dev_count--;
    if (i2c_bus[i2c->id].dev_count < 0)
        i2c_bus[i2c->id].dev_count = 0;

    i2c->id = I2C_BUS_MAX;
    pthread_mutex_unlock(&init_lock);
}

struct i2c_manager i2c_manager = {
    .init   = i2c_init,
    .deinit = i2c_deinit,
    .read   = i2c_read,
    .write  = i2c_write,
};

struct i2c_manager *get_i2c_manager(void) {
    return &i2c_manager;
}
