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

 #include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
# include <unistd.h>
# include <utime.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <types.h>
#include <utils/log.h>
 #include <utils/assert.h>
#include <lib/uart/libserialport_config.h>
#include <lib/uart/libserialport.h>
#include <lib/uart/libserialport_internal.h>
#include <uart/uart_manager.h>

#define LOG_TAG "uart"

static struct uart_port_dev port_dev[UART_MAX_CHANNELS];

static  struct uart_port_dev* get_in_use_channel(char *devname) {
    struct uart_port_dev *dev;
    int i;

    assert_die_if(devname == NULL, "devname is null\n");
    for (i = 0; i < sizeof(port_dev)/sizeof(port_dev[0]); i++) {
        dev = &port_dev[i];
        if (dev->in_use && !strcmp(devname, dev->name)) {
            return dev;
        }
    }
    return NULL;
}

static struct uart_port_dev* get_prepared_channel(char *devname) {
    struct uart_port_dev *dev;
    for (int i = 0; i < sizeof(port_dev)/sizeof(port_dev[0]); i++) {
        dev = &port_dev[i];
        if (!dev->in_use){
            strncpy(dev->name, devname, sizeof(dev->name));
            dev->name[sizeof(dev->name) - 1] = '\0';
            dev->in_use = 1;
            return dev;
        }
    }
    return NULL;
}

static struct uart_port_dev* alloc_channel(char *devname) {
    struct uart_port_dev *dev;

    if (get_in_use_channel(devname)) {
        LOGE("uart device %s is already in use\n", devname);
        return NULL;
    }

    if ((dev = get_prepared_channel(devname)) == NULL) {
        LOGE("Cannot get any useable channel\n");
        return NULL;
    }

    return dev;
}

static int free_channel(char *devname) {
    struct uart_port_dev *dev;

    if ((dev = get_in_use_channel(devname)) == NULL) {
        LOGE("uart device %s is not allocated yet\n", devname);
        return -1;
    }

    dev->in_use = 0;
    return 0;
}

static int32_t uart_init(char* devname, uint32_t baudrate, uint8_t date_bits, uint8_t parity,
        uint8_t stop_bits) {

    struct uart_port_dev *dev;

    dev = alloc_channel(devname);
    if (dev == NULL){
        LOGE("Cannot alloc channel on port %s\n", devname);
        goto out;
    }

    if (sp_new_config(&dev->sp_config) < 0) {
        LOGE("Failed to create new config\n");
        goto out;
    }

    if (sp_get_port_by_name(devname, &dev->sp_port) < 0) {
        LOGE("Failed to get port on %s\n", devname);
        goto out;
    }

    if (sp_open(dev->sp_port, SP_MODE_READ_WRITE) < 0) {
        LOGE("Failed to open port %s\n", devname);
        goto out;
    }

    if (sp_get_config(dev->sp_port, dev->sp_config) < 0) {
        LOGE("Failed to get current port config\n");
        goto out;
    }

    if (sp_set_baudrate(dev->sp_port, baudrate) < 0) {
        LOGE("Failed to set baudrate\n");
        goto out;
    }

    if (sp_set_config_bits(dev->sp_config, date_bits) < 0) {
        LOGE("Failed to set data bits\n");
        goto out;
    }

    if (sp_set_parity(dev->sp_port, parity) < 0) {
        LOGE("Failed to set parity\n");
        goto out;
    }

    if (sp_set_stopbits(dev->sp_port, stop_bits) < 0) {
        LOGE("Failed to set stop bits\n");
        goto out;
    }

    return 0;
out:
    if (dev->sp_config) {
        sp_free_config(dev->sp_config);
        dev->sp_config = NULL;
    }
    if (dev->sp_port) {
        sp_free_port(dev->sp_port);
        dev->sp_port = NULL;
    }

    free_channel(devname);
    return -1;
}

static int32_t uart_deinit(char* devname) {

    struct uart_port_dev* dev;
    if (!(dev = get_in_use_channel(devname))) {
        LOGE("uart device %s is not in use\n", devname);
        return -1;
    }

    if (sp_close(dev->sp_port) < 0) {
        LOGE("Failed to close\n");
        return -1;
    }
    if (dev->sp_config) {
        sp_free_config(dev->sp_config);
        dev->sp_config = NULL;
    }
    if (dev->sp_port) {
        sp_free_port(dev->sp_port);
        dev->sp_port = NULL;
    }
    free_channel(devname);
    return 0;
}

static int32_t uart_flow_control(char* devname, uint8_t flow_ctl) {
    struct uart_port_dev* dev;

    if (!(dev = get_in_use_channel(devname))) {
        LOGE("uart device %s is not in use\n", devname);
        goto out;
    }
    if (sp_set_flowcontrol(dev->sp_port, flow_ctl) < 0) {
        LOGE("failed to issue flow control on uart %s\n", devname);
        goto out;
    }
    return 0;
out:
    return -1;
}

static int32_t uart_write(char* devname, const void* buf, uint32_t count,
        uint32_t timeout_ms) {
    struct uart_port_dev* dev;
    uint32_t writen = 0;

    assert_die_if(buf == NULL, "parameter buf is null\n");
    if (!(dev = get_in_use_channel(devname))) {
        LOGE("uart device %s is not in use\n", devname);
        goto out;
    }

    if (((writen  = sp_blocking_write(dev->sp_port, buf, count, timeout_ms)) < 0)
        || (writen < count)) {
        LOGE("Failed to write on uart %s\n", devname);
        goto out;
    }

    if (sp_drain(dev->sp_port) < 0) {
        LOGE("Failed to wait out buffer drain\n");
        goto out;
    }

    return writen;
out:
    return -1;
}

static int32_t uart_read(char* devname, void* buf, uint32_t count, uint32_t timeout_ms) {
    struct uart_port_dev* dev;
    int32_t lower_buffer_count;
    uint32_t read_count = 0;

    assert_die_if(buf == NULL, "parameter buf is null\n");
    if (!(dev = get_in_use_channel(devname))) {
        LOGE("uart device %s is not in use\n", devname);
        goto out;
    }
    lower_buffer_count = sp_input_waiting(dev->sp_port);
    if (lower_buffer_count < 0) {
        LOGE("Failed to wait input\n");
        goto out;
    }

    if ((read_count = sp_blocking_read(dev->sp_port, buf, count, timeout_ms)) < 0) {
        LOGE("Failed to read on uart %s\n", devname);
        goto out;
    }
    if (read_count < count) {
        LOGW("timeout occured, but read size is not sufficient\n");
    }

    return read_count;
out:
    return -1;
}

struct uart_manager uart_manager = {
    .init   = uart_init,
    .deinit = uart_deinit,
    .flow_control = uart_flow_control,
    .write  = uart_write,
    .read   = uart_read,
};

struct uart_manager* get_uart_manager(void) {
    return &uart_manager;
}