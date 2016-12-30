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
#include <unistd.h>
#include <utime.h>
#include <limits.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <types.h>
#include <utils/log.h>
 #include <utils/assert.h>
#include <utils/common.h>
#include <usb/usb_manager.h>

#define LOG_TAG "usb_device_manager"

static struct usb_dev usb_dev[USB_DEVICE_MAX_COUNT];

static struct usb_dev_modules usb_dev_modules[] = {
    {USB_DEV_HID_NODE_PREFIX, USB_DEV_HID_DRIVER_MODULE, USB_DEV_HID_MAX_PKT_SIZE},
    {USB_DEV_CDC_ACM_NODE_PREFIX, USB_DEV_CDC_ACM_DRIVER_MODULE, USB_DEV_CDC_ACM_MAX_WRITE_SIZE},
};


static int32_t system_wrapped(char *cmd,  int32_t repeatcnt) {
    int32_t repeat = repeatcnt;
    int32_t retval = -1;

    while(repeat-- > 0) {
        retval = system(cmd);
        if (WIFSIGNALED(retval)
            && (WTERMSIG(retval) == SIGINT || WTERMSIG(retval) == SIGQUIT)) {
            LOGW("Unexpected interrupt\n");
            continue;
        }
        if (retval  < 0) {
            LOGE("System call on \'system\'' is failed: %s\n", strerror(errno));
            return -1;
        }
        return 0;
    }
    if (repeat <= 0) {
            LOGE("System call on \'system\'' retry count is reached\n");
            return -1;
    }
    return 0;
}
/*find module named 's' in /proc/modules*/
static int32_t is_module_installed(char *s) {
    char *file_to_open = "/proc/modules";
    FILE *fp = NULL;
    char line[PATH_MAX];
    int32_t ret = -1;
    fp = fopen(file_to_open, "r");
    if (fp == NULL) {
        LOGE("Cannot open file on %s\n", file_to_open);
        goto out;
    }

    while(fgets(line, sizeof(line), fp)) {
        if (strstr(line, s)) {
            ret = 0;
            goto out;
        }
    }
out:
    if (fp) {
        fclose(fp);
        fp = NULL;
    }
    return ret;
}

static void dump_darray(char **array, uint32_t count) {
#ifdef USB_DEBUG
    int i;
    LOGI("The whole dynamic array: \n");
    for (i = 0; i < count; i++) {
        LOGI("%d: %s \n", i, array[i]);
    }
#endif
}

static void free_darray(char ***array, uint32_t *count) {
    char **ptr = *array;
    int i;
    if (!ptr) {
        LOGW("Free darray: ptr is null\n");
        return;
    }
    for (i = 0; i < *count; i++) {
        if (ptr[i]) {
            // LOGV("free: %s\n", ptr[i]);
            free(ptr[i]);
            ptr[i] = NULL;
        }
    }
    // LOGV("free ptr: %p\n", ptr);
    free(ptr);
    *array = NULL;
    *count = 0;
}

/* split one string to array by fixed separator , max string length is equal to PATH_MAX*/
static int32_t split_string_to_darray(char *str, char *separator,
            char ***array, uint32_t *array_size) {
    int unit = sizeof(uint32_t*);
    int toklen = unit;
    char line[PATH_MAX];
     char *tok = NULL;
    char **arr_in = NULL, **arr_in_org =  NULL;
    uint32_t count = 0;
    int i= 0;
    if (strlen(str) > (sizeof(line) - 1)) {
        LOGE("String length is overlow than pedefined %d\n", (int)sizeof(line));
        return -1;
    }
    strcpy(line, str);
    tok = strtok(line, separator);
    for (; tok != NULL; tok = strtok(NULL, separator)) {
        arr_in = realloc(arr_in_org, toklen);
        if (arr_in == NULL) {
            LOGE("Cannot alloc more memory\n");
            count = i;
            goto out;
        }
        arr_in[i] = strdup(tok);
        // LOGV("alloc: %s,  ptr: %p\n", arr_in[i], arr_in);
        i++;
        toklen += unit;
        arr_in_org = arr_in;
    }
    count = i;
    *array = arr_in;
    *array_size = count;
    dump_darray(*array, *array_size);
    return 0;
out:
    free_darray(&arr_in_org, &count);
    return -1;
}

static int32_t install_base_modules(void) {
    char *usb_dev_base_modules = USB_DEV_BASE_MODULE;
    struct usb_dev *dev = NULL;
    uint32_t max_support = USB_DEVICE_MAX_COUNT;
    uint32_t users = 0;
    char buf[NAME_MAX];
    int i;

    for (i = 0; i < max_support; i++) {
        dev = &usb_dev[i];
        if (dev->in_use) {
            users++;
        }
    }
    if (users != 1) {
        return 0;
    }

    if (!is_module_installed(usb_dev_base_modules)) {
        LOGW("module %s is already installed\n", usb_dev_base_modules);
        return 0;
    }
    sprintf(buf, "modprobe %s", usb_dev_base_modules);
    // LOGV("%s\n", buf);
    if (system_wrapped(buf, USB_SWITCH_DEVICE_RETRY_COUNT) < 0) {
        LOGE("Cannot execute cmd %s\n", buf);
        return -1;
    }
    return 0;
}

static int32_t uninstall_base_modules(void) {
    char *usb_dev_base_modules = USB_DEV_BASE_MODULE;
    uint32_t max_support = USB_DEVICE_MAX_COUNT;
    struct usb_dev *dev = NULL;
    uint32_t users = 0;
    char buf[NAME_MAX];
    int i;

    for (i = 0; i < max_support; i++) {
        dev = &usb_dev[i];
        if (dev->in_use) {
            users++;
        }
    }
    if (users != 0) {
        return 0;
    }

    if (is_module_installed(usb_dev_base_modules)) {
        LOGI("module %s is already uninstalled\n", usb_dev_base_modules);
        return 0;
    }
    sprintf(buf, "rmmod %s", usb_dev_base_modules);
    // LOGV("%s\n", buf);
    if (system_wrapped(buf, USB_SWITCH_DEVICE_RETRY_COUNT) < 0) {
        LOGE("Cannot execute cmd %s\n", buf);
        return -1;
    }
    return 0;
}

static struct usb_dev_modules * get_dev_class(char *devname) {
    struct usb_dev_modules *global = usb_dev_modules;
    struct usb_dev_modules *dev_c = NULL;
    int32_t global_count =  ARRAY_SIZE(usb_dev_modules);
    int32_t i;

    for (i = 0; i < global_count; i++) {
        if (!strncmp(devname, global[i].dev, strlen(global[i].dev))) {
            dev_c = &global[i];
            break;
        }
    }
    if (dev_c == NULL) {
        LOGE("Cannot find device %s\n", devname);
        return NULL;
    }
    return dev_c;
}

static int32_t uninstall_modules(struct usb_dev *dev) {
    char **array = dev->darray.element;
    uint32_t arraysize = dev->darray.size;
    char buf[NAME_MAX];
    int32_t i;

    for (i = 0; i < arraysize; i++) {
        if (is_module_installed(array[i])) {
            // LOGV("module %s is uninstalled\n", array[i]);
            continue;
        }
        sprintf(buf, "rmmod %s", array[i]);
        // LOGV("%s\n", buf);
        if (system_wrapped(buf, USB_SWITCH_DEVICE_RETRY_COUNT) < 0) {
            LOGE("Cannot execute cmd %s\n", buf);
            goto out;
        }
    }
    free_darray(&(dev->darray.element), &(dev->darray.size));
    return 0;
out:
    free_darray(&(dev->darray.element), &(dev->darray.size));
    return -1;
}

static int32_t install_modules(struct usb_dev *dev) {
    struct usb_dev_modules *dev_c = NULL;
    char **array = NULL;
    uint32_t arraysize = 0;
    char buf[NAME_MAX];

    if ((dev_c = get_dev_class(dev->name)) == NULL) {
        LOGE("Cannot find device class %s\n", dev->name);
        return -1;
    }
    if (split_string_to_darray(dev_c->module,
            USB_DRIVER_MODULE_DEFAULT_SEPRATOR, &array, &arraysize) < 0) {
        LOGE("Cannot split string to dynamic array\n");
        return -1;
    }
    dev->darray.element = array;
    dev->darray.size = arraysize;
    if (!is_module_installed(array[0])) {
        LOGI("module %s is already installed\n", array[0]);
        return 0;
    }
    sprintf(buf, "modprobe %s", array[0]);
    // LOGV("%s\n", buf);
    if (system_wrapped(buf, USB_SWITCH_DEVICE_RETRY_COUNT) < 0) {
        LOGE("Cannot execute cmd %s\n", buf);
        return -1;
    }
    printf("sleep %d\n", USB_SWITCH_DEVICE_WAITTIME_MS);
    msleep(USB_SWITCH_DEVICE_WAITTIME_MS);
#if 0
    for (i = 0; i < arraysize; i++) {
        sprintf(buf, "insmod %s", array[i]);
        LOGI("%s\n", buf);

        if (system_wrapped(buf, SWITCH_MODULE_RETRY_COUNT) < 0) {
            LOGE("Cannot execute cmd %s\n", buf);
            return -1;
        }
    }
#endif
    return 0;
}

static int32_t gadget_switch(struct usb_dev *dev){
    int32_t dev_count =  ARRAY_SIZE(usb_dev);
    struct usb_dev *dev_exist = NULL;
    int32_t retval;
    int i;

    for (i = 0; i < dev_count; i++) {
        dev_exist = &usb_dev[i];
        if (!dev_exist->in_use)
            continue;
        /* bypass current module */
        if (i == (dev->id - 1))
            continue;
        retval = uninstall_modules(dev_exist);
        if (retval < 0) {
            LOGE("Uninstall modules on dev %s failed\n", dev_exist->name);
            return -1;
        }
    }

    retval = install_modules(dev);
    if (retval < 0) {
        LOGE("Install modules on dev %s failed\n", dev->name);
        return -1;
    }
    return 0;
}

static  struct usb_dev* get_in_use_channel(char *devname) {
    struct usb_dev *dev = NULL;
    uint32_t max_support = USB_DEVICE_MAX_COUNT;
    int i;

    assert_die_if(devname == NULL, "devname is null\n");
    for (i = 0; i < max_support; i++) {
        dev = &usb_dev[i];
        if (dev->in_use && !strcmp(devname, dev->name)) {
            return dev;
        }
    }
    return NULL;
}

static struct usb_dev* assign_channel(char *devname) {
    struct usb_dev *dev = NULL;
    uint32_t max_support = USB_DEVICE_MAX_COUNT;

    for (int i = 0; i < max_support; i++) {
        dev = &usb_dev[i];
        if (!dev->in_use){
            strncpy(dev->name, devname, sizeof(dev->name));
            dev->name[sizeof(dev->name) - 1] = '\0';
            dev->in_use = true;
            dev->id = i + 1;
            return dev;
        }
    }
    return NULL;
}

static struct usb_dev* alloc_channel(char *devname) {
    struct usb_dev *dev = NULL;

    if (get_in_use_channel(devname)) {
        LOGE("Device %s is already in use\n", devname);
        return NULL;
    }

    if ((dev = assign_channel(devname)) == NULL) {
        LOGE("Cannot get any useable channel\n");
        return NULL;
    }

    if (install_base_modules() < 0) {
        LOGE("Cannot install base modules\n");
        return NULL;
    }

    if (gadget_switch(dev) < 0) {
        LOGE("Cannot switch device\n");
        return NULL;
    }
    return dev;
}

static int32_t free_channel(char *devname) {
    struct usb_dev *dev = NULL;

    if ((dev = get_in_use_channel(devname)) == NULL) {
        LOGE("Device %s is not allocated yet\n", devname);
        return -1;
    }
    uninstall_modules(dev);
    memset(dev, 0, sizeof(*dev));
    dev->in_use = false;
    uninstall_base_modules();
    return 0;
}

static int32_t usb_device_init(char* devname) {
    struct usb_dev *dev = NULL;
    int32_t fd = -1;
    if ((dev = alloc_channel(devname)) == NULL) {
        LOGE("Cannot alloc channel for %s\n", devname);
        goto out;
    }
    if ((fd = open(devname, O_RDWR)) == -1) {
        LOGE("Cannot open device %s: %s\n",
                        devname, strerror(errno));
        free_channel(devname);
        goto out;
    }
    dev->fd = fd;
    return 0;
out:
    if (fd > 0) {
        close(fd);
        fd = -1;
    }
    return -1;
}

static int32_t usb_device_deinit(char* devname) {
    struct usb_dev *dev = NULL;

    if ((dev = get_in_use_channel(devname)) == NULL) {
        LOGE("Device %s is not in use\n", devname);
        return -1;
    }
    if (dev->fd > 0) {
        close(dev->fd);
        dev->fd = -1;
    }
    free_channel(devname);
    return 0;
}

static int32_t usb_device_switch(char* switch_to, char* switch_from) {

    if (usb_device_deinit(switch_from) < 0) {
        LOGE("Device deinit failed on %s\n", switch_from);
        return -1;
    }

    if (usb_device_init(switch_to) < 0) {
        LOGE("Device deinit failed on %s\n", switch_from);
        return -1;
    }

    return 0;
}
#if 0
static int32_t usb_hid_read_poll(char *devname, uint32_t timeout_ms) {
    struct usb_dev *dev = NULL;
    fd_set fds;
    struct timeval delta;
    int32_t result;

    if ((dev = get_in_use_channel(devname)) == NULL) {
        LOGE("Device %s is not allocated yet\n", devname);
        return -1;
    }
    delta.tv_sec = timeout_ms / 1000;
    delta.tv_usec = (timeout_ms % 1000) * 1000;

    FD_ZERO(&fds);
    FD_SET(dev->fd, &fds);
    result = select(dev->fd + 1, &fds, NULL, NULL, timeout_ms ? &delta : NULL);
    if (result < 0) {
            if (errno == EINTR) {
                LOGW("System call on select was interrupted, repeating\n");
                return 1;
            } else {
                LOGE("System call on select is failed: %s\n", strerror(errno));
                return -1;
            }
    } else if (result == 0) {
        /* Timeout has expired. */
        return -1;
    }
    return 0;
}

static int32_t usb_hid_read_unclock(char *devname, void* buf, uint32_t count) {
    struct usb_dev *dev = NULL;
    int32_t result;
    assert_die_if(buf == NULL, "Parameter buf is null\n");
    assert_die_if(count > USB_HID_MAX_PKT_SIZE,
            "Parameter count size is overlow, the maximux is %d\n", USB_HID_MAX_PKT_SIZE);

    if ((dev = get_in_use_channel(devname)) == NULL) {
        LOGE("Device %s is not allocated yet\n", devname);
        return -1;
    }
    result = read(dev->fd, buf, count);
    if (result < 0) {
        if (errno == EAGAIN){
            /* The situation is occured when system read buffer is underrun */
            return 1;
        }
        LOGE("System call on read is failed: %s\n", strerror(errno));
        return -1;
    }
    return result;
}
#endif

static int32_t usb_device_read_block(char* devname, void* buf, uint32_t count,
        uint32_t timeout_ms) {
    struct usb_dev *dev = NULL;
    struct usb_dev_modules *dev_c = NULL;
    struct timeval start, delta, now, end = {0, 0};
    int32_t started = 0;
    fd_set fds;
    int32_t result;
    char *ptr = (char *) buf;
    int32_t bytes_read = 0;

    assert_die_if(buf == NULL, "Parameter buf is null\n");

    if ((dev = get_in_use_channel(devname)) == NULL) {
        LOGE("Device %s is not allocated yet\n", devname);
        return -1;
    }

    if ((dev_c = get_dev_class(devname)) == NULL) {
        LOGE("Cannot find device class %s\n", devname);
        return -1;
    }
    assert_die_if(count > dev_c->transfer_unit,
            "Parameter count size is overlow, the maximux is %d\n",  dev_c->transfer_unit);

    if (timeout_ms) {
        gettimeofday(&start, NULL);
        delta.tv_sec = timeout_ms / 1000;
        delta.tv_usec = (timeout_ms % 1000) * 1000;
        timeradd(&start, &delta, &end);
    }

    FD_ZERO(&fds);
    FD_SET(dev->fd, &fds);
    while (bytes_read < count) {
        if (timeout_ms && started) {
            gettimeofday(&now, NULL);
            /* Timeout has expired. */
            if (timercmp(&now, &end, >))
                break;
            timersub(&end, &now, &delta);
        }
        result = select(dev->fd + 1, &fds, NULL, NULL, timeout_ms ? &delta : NULL);
        started = 1;
        if (result < 0) {
            if (errno == EINTR) {
                LOGW("System call on select was interrupted, repeating\n");
                continue;
            } else {
                LOGE("System call on select is failed: %s\n", strerror(errno));
                return -1;
            }
        } else if (result == 0) {
            /* Timeout has expired. */
            break;
        }
        result = read(dev->fd, ptr, count - bytes_read);
        if (result < 0) {
            if (errno == EAGAIN){
                /* The situation is occured when system read buffer is underrun */
                continue;
            }
            LOGE("System call on read is failed: %s\n", strerror(errno));
            return -1;
        }
        bytes_read += result;
        ptr += result;
    }
    return bytes_read;
}



static int32_t usb_device_write(char* devname, void* buf, uint32_t count,
        uint32_t timeout_ms) {

    struct usb_dev *dev = NULL;
    struct usb_dev_modules *dev_c = NULL;
    struct timeval start, delta, now, end = {0, 0};
    int started = 0;
    fd_set fds;
    int result;
    char *ptr = (char *) buf;
    int32_t bytes_writen = 0;

    assert_die_if(buf == NULL, "Parameter buf is null\n");

    if ((dev = get_in_use_channel(devname)) == NULL) {
        LOGE("Device %s is not allocated yet\n", devname);
        return -1;
    }

    if ((dev_c = get_dev_class(devname)) == NULL) {
        LOGE("Cannot find device class %s\n", devname);
        return -1;
    }
    assert_die_if(count > dev_c->transfer_unit,
            "Parameter count size is overlow, the maximux is %d\n",  dev_c->transfer_unit);

    if (timeout_ms) {
        gettimeofday(&start, NULL);
        delta.tv_sec = timeout_ms / 1000;
        delta.tv_usec = (timeout_ms % 1000) * 1000;
        timeradd(&start, &delta, &end);
    }

    FD_ZERO(&fds);
    FD_SET(dev->fd, &fds);
    while (bytes_writen < count) {
        if (timeout_ms && started) {
            gettimeofday(&now, NULL);
            /* Timeout has expired. */
            if (timercmp(&now, &end, >))
                break;
            timersub(&end, &now, &delta);
        }
        result = select(dev->fd + 1, NULL, &fds, NULL, timeout_ms ? &delta : NULL);
        started = 1;
        if (result < 0) {
            if (errno == EINTR) {
                LOGW("System call on select was interrupted, repeating\n");
                continue;
            } else {
                LOGE("System call on select is failed: %s\n", strerror(errno));
                return -1;
            }
        } else if (result == 0) {
            /* Timeout has expired. */
            break;
        }
        result = write(dev->fd, ptr, count - bytes_writen);
        if (result < 0) {
            if (errno == EAGAIN){
                /* The situation is occured when system write buffer is overrun */
                continue;
            }
            LOGE("System call on write is failed: %s\n", strerror(errno));
            return -1;
        }
        bytes_writen += result;
        ptr += result;
    }
    return bytes_writen;
}

static uint32_t usb_device_transfer_unit(char* devname) {
    struct usb_dev *dev = NULL;
    struct usb_dev_modules *dev_c = usb_dev_modules;
    uint32_t retval = 0;

    if ((dev = get_in_use_channel(devname)) == NULL) {
        LOGE("Device %s is not allocated yet\n", devname);
        return 0;
    }

    if ((dev_c = get_dev_class(devname)) == NULL) {
        LOGE("Cannot find device class %s\n", devname);
        return -1;
    }

    retval = dev_c->transfer_unit;
    // LOGV("transfer unit size is %d\n", retval);
    return retval;
}

static struct usb_device_manager usb_device_manager = {
    .init =usb_device_init,
    .deinit = usb_device_deinit,
    .switch_func = usb_device_switch,
    .get_max_transfer_unit = usb_device_transfer_unit,
    .read = usb_device_read_block,
    .write = usb_device_write,
};

struct usb_device_manager*  get_usb_device_manager(void) {
    return &usb_device_manager;
}