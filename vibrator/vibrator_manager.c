/*
 *  Copyright (C) 2016, Xin ShuAn <shuan.xin@ingenic.com>
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
#include <sys/stat.h>
#include <sys/types.h>
#include <types.h>
#include <utils/log.h>
#include <utils/assert.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <netlink/netlink_handler.h>
#include <netlink/netlink_event.h>
#include <vibrator/vibrator_manager.h>

#define IOC_MOTOR_MAGIC             'K'
#define MOTOR_SET_FUNCTION        _IO(IOC_MOTOR_MAGIC, 20)
#define MOTOR_GET_STATUS            _IO(IOC_MOTOR_MAGIC, 21)
#define MOTOR_SET_SPEED                _IO(IOC_MOTOR_MAGIC, 22)
#define MOTOR_GET_SPEED               _IO(IOC_MOTOR_MAGIC, 23)
#define MOTOR_SET_CYCLE                _IO(IOC_MOTOR_MAGIC, 24)
#define MOTOR_GET_CYCLE               _IO(IOC_MOTOR_MAGIC, 25)

#define LOG_TAG "vibrator"

struct motor_device {
    int32_t motor_id;
    int32_t motor_fd;
    motor_event_callback_t callback;
};
static uint32_t dev_num;
static uint32_t init_flag;
static pthread_mutex_t motor_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t init_lock = PTHREAD_MUTEX_INITIALIZER;
static struct motor_device motor_device[MAX_MOTOR_NUM];
static struct netlink_handler* nh;


static void handle_switch_event(struct netlink_handler* nh,
        struct netlink_event* event) {
    int i;
    uint32_t motor_id;
    const int action = event->get_action(event);
    const char* name = event->find_param(event, "SWITCH_NAME");
    const char* state = event->find_param(event, "SWITCH_STATE");
    motor_status status = MOTOR_COAST;

    if (strncmp(name, "motor",5))
        return;

    if (action == NLACTION_CHANGE) {
        if (!strcmp(state, "fault"))
            status = MOTOR_FAULT;

        motor_id = atoi(name+5);
        pthread_mutex_lock(&motor_lock);
        if(dev_num) {
            for(i = 0; i < MAX_MOTOR_NUM; i++) {
                if(motor_device[i].motor_id == motor_id) {
                    if(motor_device[i].callback)
                        motor_device[i].callback(motor_id, status);
                }
            }
        }
        pthread_mutex_unlock(&motor_lock);
    }
}

static void handle_event(struct netlink_handler* nh,
        struct netlink_event* event) {
    const char* subsystem = event->get_subsystem(event);

    if (!strcmp(subsystem, "switch"))
        event->dump(event);

    if (!strcmp(subsystem, "switch"))
        handle_switch_event(nh, event);

}

static int32_t motor_init(void) {
    int i;

    pthread_mutex_lock(&init_lock);
    if(init_flag) {
        pthread_mutex_unlock(&init_lock);
        return 0;
    }

    nh = (struct netlink_handler *) calloc(1, sizeof(struct netlink_handler));
    if(nh == NULL){
        LOGE("motor calloc netlink_handler error\n");
        pthread_mutex_unlock(&init_lock);
        return -1;
    }
    nh->construct = construct_netlink_handler;
    nh->deconstruct = destruct_netlink_handler;
    nh->construct(nh, "all sub-system", 0, handle_event, NULL);

    pthread_mutex_lock(&motor_lock);
    for(i = 0; i < MAX_MOTOR_NUM; i++) {
        motor_device[i].motor_id = -1;
        motor_device[i].motor_fd = -1;
        motor_device[i].callback = NULL;
    }
    dev_num = 0;
    pthread_mutex_unlock(&motor_lock);

    init_flag = 1;
    pthread_mutex_unlock(&init_lock);
    return 0;
}

static void motor_deinit(void) {
    int i;

    pthread_mutex_lock(&init_lock);
    if(init_flag == 0) {
        pthread_mutex_unlock(&init_lock);
        return;
    }

    pthread_mutex_lock(&motor_lock);
    for(i = 0; i < MAX_MOTOR_NUM; i++) {
        if(motor_device[i].motor_id != -1) {
            close(motor_device[i].motor_fd);
            motor_device[i].motor_id = -1;
            motor_device[i].motor_fd = -1;
            motor_device[i].callback = NULL;
        }
    }
    dev_num = 0;
    pthread_mutex_unlock(&motor_lock);

    nh->deconstruct(nh);
    free(nh);
    nh = NULL;
    init_flag = 0;
    pthread_mutex_unlock(&init_lock);
}

static int32_t motor_open(uint32_t motor_id) {
    int i, fd;
    char node[64] = "";

    pthread_mutex_lock(&motor_lock);
    for(i = 0; i < MAX_MOTOR_NUM; i++) {
        if(motor_device[i].motor_id == motor_id) {
            pthread_mutex_unlock(&motor_lock);
            return 0;
        }
    }

    if(dev_num == (MAX_MOTOR_NUM)) {
        LOGE("Open motor num more than the (MAX_MOTOR_NUM : %d)!\n", MAX_MOTOR_NUM);
        pthread_mutex_unlock(&motor_lock);
        return -1;
    }

    snprintf(node, sizeof(node), "/dev/motor%d", motor_id);
    fd = open(node, O_RDWR);
    if(fd < 0) {
        LOGE("Open %s failed: %s\n", node, strerror(errno));
        pthread_mutex_unlock(&motor_lock);
        return -1;
    }

    for(i = 0; i < MAX_MOTOR_NUM; i++) {
        if(motor_device[i].motor_id == -1) {
            motor_device[i].motor_id = motor_id;
            motor_device[i].motor_fd = fd;
            dev_num++;
            pthread_mutex_unlock(&motor_lock);
            return 0;
        }
    }

    close(fd);
    LOGE("No space to store motor device!\n");
    pthread_mutex_unlock(&motor_lock);
    return -1;
}

static void motor_close(uint32_t motor_id) {
    int i;

    pthread_mutex_lock(&motor_lock);
    for(i = 0; i < MAX_MOTOR_NUM; i++) {
        if(motor_device[i].motor_id == motor_id) {
            close(motor_device[i].motor_fd);
            motor_device[i].callback = NULL;
            motor_device[i].motor_id = -1;
            motor_device[i].motor_fd = -1;
            dev_num--;
            pthread_mutex_unlock(&motor_lock);
            return;
        }
    }
    pthread_mutex_unlock(&motor_lock);
}

static int32_t motor_set_function(uint32_t motor_id, motor_status function) {
    int i,fd = -1;
    int ret;

    pthread_mutex_lock(&motor_lock);
    for(i = 0; i < MAX_MOTOR_NUM; i++) {
        if(motor_device[i].motor_id == motor_id)
            fd = motor_device[i].motor_fd;
    }
    if(fd == -1) {
        LOGE("motor%d device is not open!\n",motor_id);
        pthread_mutex_unlock(&motor_lock);
        return -1;
    }

    ret = ioctl(fd, MOTOR_SET_FUNCTION, function);
    if(ret < 0) {
        LOGE("motor%d set function ioctl failed: %s\n", motor_id, strerror(errno));
        pthread_mutex_unlock(&motor_lock);
        return -1;
    }

    pthread_mutex_unlock(&motor_lock);
    return 0;
}

static int32_t motor_get_status(uint32_t motor_id) {
    int i,fd = -1;
    int ret;
    int function;

    pthread_mutex_lock(&motor_lock);
    for(i = 0; i < MAX_MOTOR_NUM; i++) {
        if(motor_device[i].motor_id == motor_id)
            fd = motor_device[i].motor_fd;
    }
    if(fd == -1) {
        LOGE("motor%d device is not open!\n",motor_id);
        pthread_mutex_unlock(&motor_lock);
        return -1;
    }

    ret = ioctl(fd, MOTOR_GET_STATUS, &function);
    if(ret < 0) {
        LOGE("motor%d get status ioctl failed: %s\n", motor_id, strerror(errno));
        pthread_mutex_unlock(&motor_lock);
        return -1;
    }

    pthread_mutex_unlock(&motor_lock);
    return function;
}

static int32_t motor_set_speed(uint32_t motor_id, uint32_t speed) {
    int i,fd = -1;
    int ret;

    pthread_mutex_lock(&motor_lock);
    for(i = 0; i < MAX_MOTOR_NUM; i++) {
        if(motor_device[i].motor_id == motor_id)
            fd = motor_device[i].motor_fd;
    }
    if(fd == -1) {
        LOGE("motor%d device is not open!\n",motor_id);
        pthread_mutex_unlock(&motor_lock);
        return -1;
    }

    ret = ioctl(fd, MOTOR_SET_SPEED, speed);
    if(ret < 0) {
        LOGE("motor%d set speed ioctl failed: %s\n", motor_id, strerror(errno));
        pthread_mutex_unlock(&motor_lock);
        return -1;
    }

    pthread_mutex_unlock(&motor_lock);
    return 0;
}

static int32_t motor_get_speed(uint32_t motor_id) {
    int i,fd = -1;
    int ret;
    int speed;

    pthread_mutex_lock(&motor_lock);
    for(i = 0; i < MAX_MOTOR_NUM; i++) {
        if(motor_device[i].motor_id == motor_id)
            fd = motor_device[i].motor_fd;
    }
    if(fd == -1) {
        LOGE("motor%d device is not open!\n",motor_id);
        pthread_mutex_unlock(&motor_lock);
        return -1;
    }

    ret = ioctl(fd, MOTOR_GET_SPEED, &speed);
    if(ret < 0) {
        LOGE("motor%d get speed ioctl failed: %s\n", motor_id, strerror(errno));
        pthread_mutex_unlock(&motor_lock);
        return -1;
    }

    pthread_mutex_unlock(&motor_lock);
    return speed;
}

static int32_t motor_set_cycle(uint32_t motor_id, uint32_t cycle) {
    int i,fd = -1;
    int ret;

    pthread_mutex_lock(&motor_lock);
    for(i = 0; i < MAX_MOTOR_NUM; i++) {
        if(motor_device[i].motor_id == motor_id)
            fd = motor_device[i].motor_fd;
    }
    if(fd == -1) {
        LOGE("motor%d device is not open!\n",motor_id);
        pthread_mutex_unlock(&motor_lock);
        return -1;
    }

    ret = ioctl(fd, MOTOR_SET_CYCLE, cycle);
    if(ret < 0) {
        LOGE("motor%d set cycle ioctl failed: %s\n", motor_id, strerror(errno));
        pthread_mutex_unlock(&motor_lock);
        return -1;
    }

    pthread_mutex_unlock(&motor_lock);
    return 0;
}

static int32_t motor_get_cycle(uint32_t motor_id) {
    int i,fd = -1;
    int ret;
    int cycle;

    pthread_mutex_lock(&motor_lock);
    for(i = 0; i < MAX_MOTOR_NUM; i++) {
        if(motor_device[i].motor_id == motor_id)
            fd = motor_device[i].motor_fd;
    }
    if(fd == -1) {
        LOGE("motor%d device is not open!\n",motor_id);
        pthread_mutex_unlock(&motor_lock);
        return -1;
    }

    ret = ioctl(fd, MOTOR_GET_CYCLE, &cycle);
    if(ret < 0) {
        LOGE("motor%d get cycle ioctl failed: %s\n", motor_id, strerror(errno));
        pthread_mutex_unlock(&motor_lock);
        return -1;
    }

    pthread_mutex_unlock(&motor_lock);
    return cycle;
}

static int32_t set_event_callback(uint32_t motor_id, motor_event_callback_t callback) {
    int i;

    pthread_mutex_lock(&motor_lock);
    for(i = 0; i < MAX_MOTOR_NUM; i++) {
        if(motor_device[i].motor_id == motor_id)
            motor_device[i].callback = callback;
            pthread_mutex_unlock(&motor_lock);
            return 0;
    }
    LOGE("motor%d device is not open!\n",motor_id);
    pthread_mutex_unlock(&motor_lock);
    return -1;
}

static struct netlink_handler* get_netlink_handler(void) {
    pthread_mutex_lock(&init_lock);
    if(init_flag == 0) {
        LOGE("vibrator_manager Uninitialized !\n");
        pthread_mutex_unlock(&init_lock);
        return NULL;
    }
    pthread_mutex_unlock(&init_lock);
    return nh;
}

static struct vibrator_manager vibrator_manager = {
    .init = motor_init,
    .deinit = motor_deinit,
    .open = motor_open,
    .close = motor_close,
    .set_function = motor_set_function,
    .get_status = motor_get_status,
    .set_speed = motor_set_speed,
    .get_speed = motor_get_speed,
    .set_cycle = motor_set_cycle,
    .get_cycle = motor_get_cycle,
    .set_event_callback = set_event_callback,
    .get_netlink_handler = get_netlink_handler,
};

struct vibrator_manager* get_vibrator_manager(void) {
    return &vibrator_manager;
}
