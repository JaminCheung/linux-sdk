/*
 *  Copyright (C) 2016, Zhang YanMing <jamincheung@126.com>
 *
 *  Linux recovery updater
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
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

int i2c_smbus_access(int fd, char read_write, unsigned char cmd, int size,
        union i2c_smbus_data *data) {

    struct i2c_smbus_ioctl_data args;
    int err;

    args.read_write = read_write;
    args.command = cmd;
    args.size = size;
    args.data = data;

    err = ioctl(fd, I2C_SMBUS, &args);
    if (err == -1)
        err = -errno;

    return err;
}

int i2c_smbus_read_byte(int fd) {
    union i2c_smbus_data data;
    int err;

    err = i2c_smbus_access(fd, I2C_SMBUS_READ, 0, I2C_SMBUS_BYTE, &data);
    if (err < 0)
        return err;

    return 0x0FF & data.byte;
}

int i2c_smbus_read_byte_data(int fd, unsigned char cmd) {
    union i2c_smbus_data data;
    int err;

    err = i2c_smbus_access(fd, I2C_SMBUS_READ, cmd, I2C_SMBUS_BYTE_DATA, &data);
    if (err < 0)
        return err;

    return 0x0FF & data.byte;
}

int i2c_smbus_read_word_data(int fd, unsigned char cmd) {
    union i2c_smbus_data data;
    int err;

    err = i2c_smbus_access(fd, I2C_SMBUS_READ, cmd, I2C_SMBUS_WORD_DATA, &data);
    if (err < 0)
        return err;

    return 0x0FFFF & data.word;
}

int i2c_smbus_read_block_data(int fd, unsigned char cmd, unsigned char *vals) {
    union i2c_smbus_data data;
    int i, err;

    err = i2c_smbus_access(fd, I2C_SMBUS_READ, cmd, I2C_SMBUS_BLOCK_DATA,
            &data);
    if (err < 0)
        return err;

    for (i = 1; i <= data.block[0]; i++)
        *vals++ = data.block[i];
    return data.block[0];
}

int i2c_smbus_read_i2c_block_data(int fd, unsigned char cmd, unsigned char len,
        unsigned char *vals) {
    union i2c_smbus_data data;
    int i, err;

    if (len > I2C_SMBUS_BLOCK_MAX)
        len = I2C_SMBUS_BLOCK_MAX;
    data.block[0] = len;

    err = i2c_smbus_access(fd, I2C_SMBUS_READ, cmd,
            len == 32 ? I2C_SMBUS_I2C_BLOCK_BROKEN : I2C_SMBUS_I2C_BLOCK_DATA,
            &data);
    if (err < 0)
        return err;

    for (i = 1; i <= data.block[0]; i++)
        *vals++ = data.block[i];
    return data.block[0];
}

int i2c_smbus_write_quick(int file, unsigned char value) {
    return i2c_smbus_access(file, value, 0, I2C_SMBUS_QUICK, NULL);
}

int i2c_smbus_write_byte(int fd, unsigned char val) {
    return i2c_smbus_access(fd, I2C_SMBUS_WRITE, val, I2C_SMBUS_BYTE, NULL);
}

int i2c_smbus_write_byte_data(int file, unsigned char cmd, unsigned char value) {
    union i2c_smbus_data data;

    data.byte = value;

    return i2c_smbus_access(file, I2C_SMBUS_WRITE, cmd, I2C_SMBUS_BYTE_DATA,
            &data);
}

int i2c_smbus_write_word_data(int file, unsigned char cmd, unsigned short value) {
    union i2c_smbus_data data;

    data.word = value;

    return i2c_smbus_access(file, I2C_SMBUS_WRITE, cmd, I2C_SMBUS_WORD_DATA,
            &data);
}

int i2c_smbus_write_block_data(int file, unsigned char cmd,
        unsigned char length, const unsigned char *values) {
    union i2c_smbus_data data;

    if (length > I2C_SMBUS_BLOCK_MAX)
        length = I2C_SMBUS_BLOCK_MAX;

    memcpy(data.block + 1, values, length);
    data.block[0] = length;

    return i2c_smbus_access(file, I2C_SMBUS_WRITE, cmd, I2C_SMBUS_BLOCK_DATA,
            &data);
}

int i2c_smbus_write_i2c_block_data(int file, unsigned char cmd,
        unsigned char length, const unsigned char *values) {
    union i2c_smbus_data data;

    if (length > I2C_SMBUS_BLOCK_MAX)
        length = I2C_SMBUS_BLOCK_MAX;

    memcpy(data.block + 1, values, length);
    data.block[0] = length;

    return i2c_smbus_access(file, I2C_SMBUS_WRITE, cmd,
            I2C_SMBUS_I2C_BLOCK_BROKEN, &data);
}

int i2c_smbus_set_slave_addr(int fd, int addr, bool force) {
    int retval = 0;

    if (force)
        retval = ioctl(fd, I2C_SLAVE_FORCE, (void*) (intptr_t) addr);
    else
        retval = ioctl(fd, I2C_SLAVE, (void*) (intptr_t) addr);

    return retval;
}

int i2c_smbus_process_call(int file, unsigned char cmd, unsigned short value) {
    union i2c_smbus_data data;
    data.word = value;
    if (i2c_smbus_access(file, I2C_SMBUS_WRITE, cmd, I2C_SMBUS_PROC_CALL,
            &data))
        return -1;
    else
        return 0x0FFFF & data.word;
}

int i2c_smbus_block_process_call(int file, unsigned char cmd,
        unsigned char length, unsigned char *values) {
    union i2c_smbus_data data;
    int i, err;

    if (length > I2C_SMBUS_BLOCK_MAX)
        length = I2C_SMBUS_BLOCK_MAX;
    for (i = 1; i <= length; i++)
        data.block[i] = values[i - 1];
    data.block[0] = length;

    err = i2c_smbus_access(file, I2C_SMBUS_WRITE, cmd,
            I2C_SMBUS_BLOCK_PROC_CALL, &data);
    if (err < 0)
        return err;

    for (i = 1; i <= data.block[0]; i++)
        values[i - 1] = data.block[i];

    return data.block[0];
}

int i2c_smbus_open(int i2cbus) {
    char filename[NAME_MAX] = { 0 };
    int fd;

    sprintf(filename, "/dev/i2c-%d", i2cbus);
    fd = open(filename, O_RDWR | O_SYNC);
    if (fd < 0) {
        if (errno == ENOENT) {
            filename[8] = '/';
            fd = open(filename, O_RDWR | O_SYNC);
        }
    }

    return fd;
}

int i2c_smbus_close(int fd) {
    if (fd < 0)
        return -1;

    close(fd);

    return 0;
}

int i2c_smbus_get_funcs_matrix(int fd, unsigned long *funcs) {
    int retval = 0;

    retval = ioctl(fd, I2C_FUNCS, funcs);

    return retval;
}
