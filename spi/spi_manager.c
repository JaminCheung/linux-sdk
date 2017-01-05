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
#include <linux/types.h>
#include <linux/spi/spidev.h>

#include <utils/log.h>
#include <utils/assert.h>
#include <spi/spi_manager.h>


/*
 * Macro
 */
#define LOG_TAG   "spi"

/*
 * Struct
 */
enum operation {
    SPI_READ,
    SPI_WRITE,
};

struct spi_dev {
    int fd;
    uint8_t mode;
    uint8_t bits;
    uint32_t speed;
    bool is_init;
};

static struct spi_dev spi_dev[SPI_DEVICE_MAX];

/*
 * Functions
 */
static int32_t spi_transfer(enum spi id, uint8_t *txbuf, uint8_t *rxbuf, uint32_t length) {
    assert_die_if(id >= SPI_DEVICE_MAX, "SPI-%d is invalid!\n", id);
    assert_die_if(length > SPI_MSG_LENGTH_MAX, "Error: length cannot greater than 4K\n");
    assert_die_if((!txbuf || !rxbuf), "Error: %s pointer cannot be 'NULL'\n", !txbuf ? "txbuf":"rxbuf");
    if (spi_dev[id].is_init == false) {
        LOGE("The SPI-%d is not initialized!\n", id);
        return -1;
    }

    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)txbuf,
        .rx_buf = (unsigned long)rxbuf,
        .len    = length,
        .speed_hz = spi_dev[id].speed,
        .delay_usecs = SPI_MSG_DELAY_US,
        .bits_per_word = spi_dev[id].bits,
    };

    if (ioctl(spi_dev[id].fd, SPI_IOC_MESSAGE(1), &tr) < 0) {
        LOGE("Failed to send message: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

static int32_t spi_operation(enum spi id, uint8_t *buf, uint32_t length, enum operation oprt) {
    int retval;

    assert_die_if(id >= SPI_DEVICE_MAX, "SPI-%d is invalid!\n", id);
    assert_die_if(length > SPI_MSG_LENGTH_MAX, "Error: length cannot greater than 4K\n");
    assert_die_if(!buf, "Error: %s pointer cannot be 'NULL'\n", oprt ? "txbuf":"rxbuf");
    if (spi_dev[id].is_init == false) {
        LOGE("The SPI-%d is not initialized!\n", id);
        return -1;
    }

    if (oprt) {
        retval = write(spi_dev[id].fd, buf, length);
        if (retval < 0) {
            LOGE("Failed to write SPI-%d: %s\n", id, strerror(errno));
            return -1;
        }
    } else {
        retval = read(spi_dev[id].fd, buf, length);
        if (retval < 0) {
            LOGE("Failed to read SPI-%d: %s\n", id, strerror(errno));
            return -1;
        }
    }

    return retval;
}

static int32_t spi_write(enum spi id, uint8_t *txbuf, uint32_t length) {
    return spi_operation(id, txbuf, length, SPI_WRITE);
}

static int32_t spi_read(enum spi id, uint8_t *rxbuf, uint32_t length) {
    return spi_operation(id, rxbuf, length, SPI_READ);
}

static int32_t spi_init(enum spi id, uint8_t mode, uint8_t bits, uint32_t speed) {
    int fd;
    char node[64] = "";

    assert_die_if(id >= SPI_DEVICE_MAX, "SPI-%d is invalid!\n", id);
    if (spi_dev[id].is_init == true) {
        close(spi_dev[id].fd);
        spi_dev[id].is_init = false;
    }

    snprintf(node, sizeof(node), "/dev/spidev0.%d", id);
    fd = open(node, O_RDWR);
    assert_die_if(fd < 0, "Open %s failed: %s\n", strerror(errno));

    /*
     * spi mode
     */
    if (ioctl(fd, SPI_IOC_WR_MODE, &mode) < 0) {
        LOGE("Failed to set spi mode: %s\n", strerror(errno));
        return -1;
    }

    /*
     * bits per word
     */
    if (ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits) < 0) {
        LOGE("Failed to set bits per word: %s\n", strerror(errno));
        return -1;
    }

    /*
     * max speed(Hz)
     */
    if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0) {
        LOGE("Failed to set max speed hz: %s\n", strerror(errno));
        return -1;
    }

    spi_dev[id].fd    = fd;
    spi_dev[id].mode  = mode;
    spi_dev[id].bits  = bits;
    spi_dev[id].speed = speed;
    spi_dev[id].is_init = true;

    return 0;
}

static void spi_deinit(enum spi id) {
    assert_die_if(id >= SPI_DEVICE_MAX, "SPI-%d is invalid!\n", id);
    if (spi_dev[id].is_init == false)
        return;

    close(spi_dev[id].fd);
    spi_dev[id].fd = -1;
    spi_dev[id].is_init = false;
}

struct spi_manager spi_manager = {
    .init     = spi_init,
    .deinit   = spi_deinit,
    .read     = spi_read,
    .write    = spi_write,
    .transfer = spi_transfer,
};

struct spi_manager *get_spi_manager(void) {
    return &spi_manager;
}
