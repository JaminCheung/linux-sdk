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

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <utils/log.h>
#include <lib/gpio/libgpio.h>

#define LOG_TAG "libgpio"

#define SYSFS       "/sys/class/gpio/"
#define EXPORT      SYSFS"export"
#define UNEXPORT    SYSFS"unexport"
#define STR_LEN     64

static int export_un_export(struct gpio_pin* pin, uint32_t export) {
    char c[STR_LEN];
    uint32_t len = 0;
    int ret = 0;
    int fd = 0;

    if (export)
        fd = open(EXPORT, O_WRONLY);
    else
        fd = open(UNEXPORT, O_WRONLY);

    if (fd == -1) {
        ret = errno;
        pin->valid = GPIO_INVALID;

        return -ret;
    }

    ret = snprintf(c, STR_LEN, "%d", pin->no);

    if (ret < 0) {
        pin->valid = GPIO_INVALID;
        goto close_out;
    }

    len = write (fd, c, ret);
    if (len != ret) {
        ret = -errno;
        pin->valid = GPIO_INVALID;

        goto close_out;
    }

    ret = 0;

    pin->valid = GPIO_VALID;

close_out:
    close(fd);

    return ret;
}

inline static int export(struct gpio_pin* pin) {
    return export_un_export(pin, 1);
}

inline static int unexport(struct gpio_pin* pin) {
    return export_un_export(pin, 0);
}

static int get_direction(struct gpio_pin* pin) {
    char c[STR_LEN];
    char dir;
    int fd = 0;
    int ret = 0;
    uint32_t len = 0;

    ret = snprintf(c, STR_LEN, SYSFS"gpio%d/direction", pin->no);
    if (ret < 0)
        return ret;

    ret = 0;
    fd = open(c, O_RDONLY);
    if (fd == -1) {
        ret = -errno;
        return ret;
    }

    len = read(fd, &dir, 1);
    if (len != 1) {
        ret = -errno;
        return ret;
    }

    switch(dir) {
    case 'i':
        pin->direction = GPIO_IN;
        break;

    case 'o':
        pin->direction = GPIO_OUT;
        break;

    default:
        ret = -EINVAL;
        goto close_out;
    }

close_out:
    close(fd);

    return ret;
}

static int open_value_fd(struct gpio_pin* pin) {
    char c[STR_LEN];
    int ret = 0;

    ret = snprintf(c, STR_LEN, SYSFS"gpio%d/value", pin->no);
    if (ret < 0)
        return ret;

    ret = 0;
    switch(pin->direction) {
    case GPIO_OUT:
        pin->fd = open(c, O_RDWR);
        break;

    case GPIO_IN:
        pin->fd = open(c, O_RDONLY);
        break;

    default:
        return -EINVAL;
    }

    return ret;
}

static int direction(struct gpio_pin* pin, char* dir) {
    char c[STR_LEN];
    int fd = 0;
    int ret = 0;

    ret = snprintf(c, STR_LEN, SYSFS"gpio%d/direction", pin->no);
    if (ret < 0) {
        pin->valid = GPIO_INVALID;
        return ret;
    }

    fd = open(c, O_RDWR);
    if (fd == -1) {
        ret = -errno;
        pin->valid = GPIO_INVALID;
        return ret;
    }

    ret = write(fd, dir, strnlen(dir, 3));
    if (ret != strnlen(dir, 3)) {
        ret = -errno;
        pin->valid = GPIO_INVALID;
        goto close_out;
    }

    ret = 0;

close_out:
    close(fd);

    return ret;
}

static int set_irq(struct gpio_pin *pin, gpio_irq_mode m) {
    char c[STR_LEN];
    int fd, ret = 0;

    ret = snprintf(c, STR_LEN, SYSFS"gpio%d/edge", pin->no);

    if (ret < 0)
        return ret;

    fd = open(c, O_RDWR);
    if (fd == -1) {
        ret = -errno;
        return ret;
    }

    switch (m) {
        case GPIO_RISING:
            ret = write(fd, "rising", 6);
            if (ret != 6)
                ret = -1;
            break;

        case GPIO_FALLING:
            ret = write(fd, "falling", 7);
            if (ret != 7)
                ret = -1;
            break;

        case GPIO_BOTH:
            ret = write(fd, "both", 4);
            if (ret != 4)
                ret = -1;
            break;

        case GPIO_NONE:
            ret = write(fd, "none", 4);
            if (ret != 4)
                ret = -1;
            break;

        default:
            return -1;
    }

    if (ret == -1) {
        ret = -errno;
        goto close_out;
    }

    pin->irq_mode = m;
    ret = 0;

close_out:
    close (fd);

    return ret;
}

int gpio_out(struct gpio_pin *pin) {
    int ret = 0;

    LOGD("%s: %d", __FUNCTION__, pin->no);

    ret = direction(pin, "out\0");

    if (!ret)
        pin->direction = GPIO_OUT;

    return ret;
}

int gpio_in(struct gpio_pin *pin) {
    int ret = 0;

    LOGD("%s: %d", __FUNCTION__, pin->no);

    ret = direction(pin, "in\0");

    if (!ret)
        pin->direction = GPIO_IN;

    return ret;
}

int gpio_set_value(struct gpio_pin *pin, gpio_value value) {
    int ret = 0;
    char c;

    LOGD("%s: %d = %d", __FUNCTION__, pin->no, value);

    if (pin->direction != GPIO_OUT)
        return -EINVAL;

    if (pin->valid != GPIO_VALID) {
        return -ENOMEM;
    }

    if (pin->fd == -1)
        ret = open_value_fd(pin);

    if (ret)
        return ret;

    switch (value) {
        case GPIO_LOW:
            c = '0';
            break;

        case GPIO_HIGH:
            c = '1';
            break;

        default:
            return -EINVAL;
    }

    ret = write(pin->fd, &c, 1);
    if (ret != 1) {
        ret = -errno;
        close (pin->fd);
        pin->fd = -1;
        return ret;
    }

    return 0;
}


int gpio_get_value(struct gpio_pin *pin, gpio_value *value) {
    int ret = 0;
    char c;

    LOGD("%s: %d", __FUNCTION__, pin->no);

    if (pin->valid != GPIO_VALID)
        return -ENOMEM;

    if (pin->fd == -1)
        ret = open_value_fd (pin);

    if (ret)
        return ret;

    ret = lseek(pin->fd, 0x00, SEEK_SET);
    if (ret == -1) {
        ret = -errno;
        close (pin->fd);
        pin->fd = -1;
        return ret;
    }

    ret = read(pin->fd, &c, 1);
    if (ret != 1) {
        ret = -errno;
        close (pin->fd);
        pin->fd = -1;
        return ret;
    }

    ret = 0;

    switch (c) {
        case '0':
            *value = GPIO_LOW;
            break;

        case '1':
            *value = GPIO_HIGH;
            break;

        default:
            return -EINVAL;
    }

    return ret;
}

int gpio_open(struct gpio_pin *pin, unsigned int no) {
    int ret = 0;

    LOGD("%s: %d", __FUNCTION__, no);

    pin->no = no;
    pin->fd = -1;

    ret = export(pin);

    if (ret < 0 ) {
        pin->valid = GPIO_INVALID;
        return ret;
    }

    ret = get_direction(pin);
    if (ret)
        pin->valid = GPIO_INVALID;
    else
        pin->valid = GPIO_VALID;

    return ret;
}

int gpio_open_dir(struct gpio_pin *pin, unsigned int no, gpio_direction dir) {
    int ret = 0;

    LOGD("%s: %d - %d", __FUNCTION__, no, dir);

    ret = gpio_open(pin, no);

    if (ret)
        return ret;

    switch (dir) {
        case GPIO_OUT:
            ret = gpio_out(pin);
            break;

        case GPIO_IN:
            ret = gpio_in(pin);
            break;

        default:
            ret = -EINVAL;
    }

    return ret;
}

int gpio_close(struct gpio_pin *pin) {
    int ret = 0;

    LOGD("%s: %d", __FUNCTION__, pin->no);

    if (pin->fd != -1) {
        close (pin->fd);
        pin->fd = -1;
    }

    ret = unexport(pin);

    return ret;
}

int gpio_enable_irq(struct gpio_pin *pin, gpio_irq_mode m) {
    LOGD("%s: %d", __FUNCTION__, pin->no);

    if (pin->direction == GPIO_OUT)
        return -1;

    if (pin->valid != GPIO_VALID)
        return -ENOMEM;

    return set_irq(pin, m);
}

int gpio_get_fd(struct gpio_pin *pin) {
    return pin->fd;
}


int gpio_irq_timed_wait(struct gpio_pin *pin, gpio_value *value, int timeout_ms) {
    int ret = 0;
    struct pollfd irqdesc = {
        .events = POLLPRI | POLLERR ,
    };

    if (pin->valid != GPIO_VALID)
        return -ENOMEM;

    if (pin->fd == -1)
        ret = open_value_fd(pin);

    if (ret)
        return ret;

    irqdesc.fd = pin->fd;

    ret = poll(&irqdesc, 1, timeout_ms);
    if (ret == -1) {
        ret = -errno;
        close (pin->fd);
        pin->fd = -1;
        return ret;
    }

    /* timeout */
    if (ret == 0)
        return -1;

    return gpio_get_value(pin, value);
}

int gpio_irq_wait(struct gpio_pin *pin, gpio_value *value) {
    return gpio_irq_timed_wait(pin, value, -1);
}
