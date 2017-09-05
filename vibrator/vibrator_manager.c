/*
 *  Copyright (C) 2016, Xin ShuAn <shuan.xin@ingenic.com>
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
#include <utils/list.h>
#include <netlink/netlink_handler.h>
#include <netlink/netlink_event.h>
#include <vibrator/vibrator_manager.h>

#define IOC_MOTOR_MAGIC             'K'
#define MOTOR_SET_FUNCTION        _IO(IOC_MOTOR_MAGIC, 20)
#define MOTOR_GET_STATUS             _IO(IOC_MOTOR_MAGIC, 21)
#define MOTOR_SET_SPEED                _IO(IOC_MOTOR_MAGIC, 22)
#define MOTOR_GET_SPEED               _IO(IOC_MOTOR_MAGIC, 23)
#define MOTOR_SET_CYCLE                _IO(IOC_MOTOR_MAGIC, 24)
#define MOTOR_GET_CYCLE               _IO(IOC_MOTOR_MAGIC, 25)

#define LOG_TAG "vibrator"

typedef enum _motor_ioctl_direction {
    MOTOR_IOCTL_SET = 0x0,
    MOTOR_IOCTL_GET = 0x01,
}motor_ioctl_direction;

struct event_callback_t {
    motor_event_callback_t callback;
    struct list_head node;
};

struct motor_device_t {
    int32_t motor_id;
    int32_t motor_fd;
    struct list_head callback_head;
    struct list_head device_node;
};

static struct netlink_handler* netlink_handler;
static LIST_HEAD(device_head);

static pthread_mutex_t init_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t device_lock = PTHREAD_MUTEX_INITIALIZER;

static void handle_switch_event(struct netlink_handler* nh,
        struct netlink_event* event) {
    uint32_t motor_id;
    struct motor_device_t *motor_device;
    struct event_callback_t *event_callback;
    struct list_head* device_pos;
    struct list_head* callback_pos;
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

        pthread_mutex_lock(&device_lock);
        list_for_each(device_pos, &device_head) {
            motor_device = list_entry(device_pos, struct motor_device_t, device_node);
            if (motor_device->motor_id == motor_id) {
                list_for_each(callback_pos, &motor_device->callback_head) {
                    event_callback = list_entry(callback_pos, struct event_callback_t, node);
                    event_callback->callback(motor_id, status);
                }
                pthread_mutex_unlock(&device_lock);
                return;
            }
        }
        pthread_mutex_unlock(&device_lock);
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
    pthread_mutex_lock(&init_lock);

    if(netlink_handler == NULL) {
        netlink_handler = (struct netlink_handler *) calloc(1, sizeof(struct netlink_handler));
        if(netlink_handler == NULL){
            LOGE("vibrator manager init error\n");
            pthread_mutex_unlock(&init_lock);
            return -1;
        }
        netlink_handler->construct = construct_netlink_handler;
        netlink_handler->deconstruct = destruct_netlink_handler;
        netlink_handler->construct(netlink_handler, "all sub-system", 0, handle_event, NULL);
    }

    pthread_mutex_unlock(&init_lock);
    return 0;
}

static void motor_deinit(void) {
    struct list_head* device_pos;
    struct list_head* device_next_pos;
    struct list_head* callback_pos;
    struct list_head* callback_next_pos;
    struct motor_device_t *motor_device;
    struct event_callback_t *event_callback;

    pthread_mutex_lock(&device_lock);
    list_for_each_safe(device_pos, device_next_pos, &device_head) {
        motor_device = list_entry(device_pos, struct motor_device_t, device_node);

        list_for_each_safe(callback_pos, callback_next_pos, &motor_device->callback_head) {
            event_callback = list_entry(callback_pos, struct event_callback_t, node);
            list_del(&event_callback->node);
            free(event_callback);
        }
        close(motor_device->motor_fd);
        list_del(&motor_device->device_node);
        free(motor_device);
    }
    pthread_mutex_unlock(&device_lock);

    pthread_mutex_lock(&init_lock);
    netlink_handler->deconstruct(netlink_handler);
    free(netlink_handler);
    netlink_handler = NULL;
    pthread_mutex_unlock(&init_lock);
}

static int32_t motor_open(uint32_t motor_id) {
    int fd;
    char node[64] = "";
    struct list_head* pos;
    struct motor_device_t *motor_device;

    pthread_mutex_lock(&device_lock);
    list_for_each(pos, &device_head) {
        motor_device = list_entry(pos, struct motor_device_t, device_node);
        if (motor_device->motor_id == motor_id) {
            pthread_mutex_unlock(&device_lock);
            return 0;
        }
    }

    snprintf(node, sizeof(node), "/dev/motor%d", motor_id);
    fd = open(node, O_RDWR);
    if(fd < 0) {
        LOGE("Open %s failed: %s\n", node, strerror(errno));
        pthread_mutex_unlock(&device_lock);
        return -1;
    }

    motor_device = malloc(sizeof(struct motor_device_t));
    if(motor_device == NULL) {
        LOGE("motor_open: malloc motor_device fail\n");
        close(fd);
        pthread_mutex_unlock(&device_lock);
        return -1;
    }

    motor_device->motor_id = motor_id;
    motor_device->motor_fd = fd;
    INIT_LIST_HEAD(&motor_device->callback_head);
    list_add_tail(&motor_device->device_node, &device_head);

    pthread_mutex_unlock(&device_lock);
    return 0;
}

static void motor_close(uint32_t motor_id) {
    struct list_head* device_pos;
    struct list_head* device_next_pos;
    struct list_head* callback_pos;
    struct list_head* callback_next_pos;
    struct motor_device_t *motor_device;
    struct event_callback_t *event_callback;

    pthread_mutex_lock(&device_lock);
    list_for_each_safe(device_pos, device_next_pos, &device_head) {
        motor_device = list_entry(device_pos, struct motor_device_t, device_node);
        if(motor_device->motor_id == motor_id) {
            list_for_each_safe(callback_pos, callback_next_pos, &motor_device->callback_head) {
                event_callback = list_entry(callback_pos, struct event_callback_t, node);
                list_del(&event_callback->node);
                free(event_callback);
            }
            close(motor_device->motor_fd);
            list_del(&motor_device->device_node);
            free(motor_device);
            pthread_mutex_unlock(&device_lock);
            return;
        }
    }

    pthread_mutex_unlock(&device_lock);
}

static int32_t motor_ioctl(uint32_t motor_id, int32_t command,
        motor_ioctl_direction dir, int function) {
    int error;
    int data = 0;
    struct list_head* pos;
    struct motor_device_t *motor_device;

    pthread_mutex_lock(&device_lock);
    list_for_each(pos, &device_head) {
        motor_device = list_entry(pos, struct motor_device_t, device_node);
        if (motor_device->motor_id == motor_id) {
            if(dir == MOTOR_IOCTL_SET)
                error = ioctl(motor_device->motor_fd, command, function);
            else
                error = ioctl(motor_device->motor_fd, command, &data);

            if(error) {
                LOGE("motor%d %X ioctl failed: %s\n", motor_id, command, strerror(errno));
                pthread_mutex_unlock(&device_lock);
                return -1;
            }
            pthread_mutex_unlock(&device_lock);
            return data;
        }
    }

    LOGE("motor%d device is not open!\n",motor_id);
    pthread_mutex_unlock(&device_lock);
    return -1;
}

static int32_t motor_set_function(uint32_t motor_id, motor_status function) {
    return motor_ioctl(motor_id, MOTOR_SET_FUNCTION, MOTOR_IOCTL_SET, function);
}

static int32_t motor_get_status(uint32_t motor_id) {
    return motor_ioctl(motor_id, MOTOR_GET_STATUS, MOTOR_IOCTL_GET, 0);
}

static int32_t motor_set_speed(uint32_t motor_id, uint32_t speed) {
    return motor_ioctl(motor_id, MOTOR_SET_SPEED, MOTOR_IOCTL_SET, speed);
}

static int32_t motor_get_speed(uint32_t motor_id) {
    return motor_ioctl(motor_id, MOTOR_GET_SPEED, MOTOR_IOCTL_GET, 0);
}

static int32_t motor_set_cycle(uint32_t motor_id, uint32_t cycle) {
    return motor_ioctl(motor_id, MOTOR_SET_CYCLE, MOTOR_IOCTL_SET, cycle);
}

static int32_t motor_get_cycle(uint32_t motor_id) {
    return motor_ioctl(motor_id, MOTOR_GET_CYCLE, MOTOR_IOCTL_GET, 0);
}

static int32_t register_event_callback(uint32_t motor_id, motor_event_callback_t callback) {
    struct list_head* pos;
    struct motor_device_t *motor_device;
    struct event_callback_t *event_callback;
    assert_die_if(callback == NULL, "callback is NULL\n");

    pthread_mutex_lock(&device_lock);
    list_for_each(pos, &device_head) {
        motor_device = list_entry(pos, struct motor_device_t, device_node);
        if (motor_device->motor_id == motor_id) {
            event_callback = malloc(sizeof(struct event_callback_t));
            if(event_callback == NULL) {
                LOGE("register_event_callback: malloc event_callback fail\n");
                pthread_mutex_unlock(&device_lock);
                return -1;
            }
            event_callback->callback = callback;
            list_add_tail(&event_callback->node, &motor_device->callback_head);
            pthread_mutex_unlock(&device_lock);
            return 0;
        }
    }

    LOGE("motor%d device is not open!\n",motor_id);
    pthread_mutex_unlock(&device_lock);
    return -1;
}

static int32_t unregister_event_callback(uint32_t motor_id, motor_event_callback_t callback) {
    struct list_head* device_pos;
    struct list_head* callback_pos;
    struct list_head* callback_next_pos;
    struct motor_device_t *motor_device;
    struct event_callback_t *event_callback;
    assert_die_if(callback == NULL, "callback is NULL\n");

    pthread_mutex_lock(&device_lock);
    list_for_each(device_pos, &device_head) {
        motor_device = list_entry(device_pos, struct motor_device_t, device_node);
        if (motor_device->motor_id == motor_id) {
            list_for_each_safe(callback_pos, callback_next_pos, &motor_device->callback_head) {
                event_callback = list_entry(callback_pos, struct event_callback_t, node);
                if(event_callback->callback == callback) {
                    list_del(&event_callback->node);
                    free(event_callback);
                    pthread_mutex_unlock(&device_lock);
                    return 0;
                }
            }
            LOGE("motor%d: not find callback!\n", motor_id);
            pthread_mutex_unlock(&device_lock);
            return -1;
        }
    }

    LOGE("motor%d device is not open!\n", motor_id);
    pthread_mutex_unlock(&device_lock);
    return -1;
}

static struct netlink_handler* get_netlink_handler(void) {
    return netlink_handler;
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
    .register_event_callback = register_event_callback,
    .unregister_event_callback = unregister_event_callback,
    .get_netlink_handler = get_netlink_handler,
};

struct vibrator_manager* get_vibrator_manager(void) {
    return &vibrator_manager;
}
