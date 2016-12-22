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
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <utils/assert.h>
#include <utils/list.h>
#include <utils/log.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <gpio/gpio_manager.h>

#define LOG_TAG "gpio"

#define MAX_POLL_NUM                GPIO_PD(5)+1
#define IRQ_PTHREAD_MODE         SCHED_RR

static int epollfd;
static gpio_irq_func irq_func;
static pthread_t irq_pthread_id;
static bool init_flag;
static struct gpio_pin pin[MAX_POLL_NUM];


static void *gpio_pthread_func(void *data) {
    int num;
    struct epoll_event events[4];

    for(;;) {
        num = epoll_wait(epollfd, events, 4, -1);

        while(0 < num--)
            irq_func(events[num].data.u32);
    }

    return  NULL;
}

static void set_irq_func(gpio_irq_func func) {
    assert_die_if(func == NULL, "func is null\n");
    irq_func = func;
}

static int32_t init(void) {
    struct sched_param param;
    pthread_attr_t pthread_attr;

    if(init_flag == false) {
        epollfd = epoll_create(MAX_POLL_NUM);
        if (epollfd == -1) {
            LOGE("Failed to init gpio manager\n");
            return -1;
        }

        /*init prhread_attr*/
        pthread_attr_init(&pthread_attr);

        /*set pthread_attr scope ,  speed up the reaction*/
        pthread_attr_setscope(&pthread_attr, PTHREAD_SCOPE_SYSTEM);

        /*set irq_pthread  real time scheduling strategy*/
        pthread_attr_setschedpolicy(&pthread_attr, IRQ_PTHREAD_MODE);

        /*set irq_pthread highest priority*/
        param.__sched_priority = sched_get_priority_max(IRQ_PTHREAD_MODE);
        pthread_attr_setschedparam(&pthread_attr, &param);

        pthread_attr_setinheritsched(&pthread_attr, PTHREAD_EXPLICIT_SCHED);

        if(pthread_create(&irq_pthread_id, &pthread_attr, gpio_pthread_func, NULL) != 0)
            assert_die_if(true, "gpio irq pthread create error\n");

        pthread_attr_destroy(&pthread_attr);
        init_flag = true;
    }
    return 0;
}

static int32_t enable_irq(uint32_t gpio, gpio_irq_mode mode) {
    struct epoll_event ev;
    gpio_value value;
    assert_die_if(gpio >= MAX_POLL_NUM, "gpio%d enable irq , number exceeds the threshold\n", gpio);
    assert_die_if(mode == GPIO_NONE, "gpio%d set irq mode is none\n", gpio);

    if(init_flag == false) {
        if(init() < 0)
            goto gpio_enable_irq_err;
    }

    if(irq_func == NULL)
        goto gpio_enable_irq_err;

    if (pin[gpio].fd == -1) {
        if(gpio_get_value(&pin[gpio], &value) < 0)
            goto gpio_enable_irq_err;
    }

    if(gpio_enable_irq(&pin[gpio], mode) < 0)
        goto gpio_enable_irq_err;

    ev.events = EPOLLPRI | EPOLLET;
    ev.data.u32 = gpio;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, pin[gpio].fd, &ev);
    return 0;

gpio_enable_irq_err:
    LOGE("Failed to gpio%d enable irq\n", gpio);
    return -1;
}

static void disable_irq(uint32_t gpio) {
    assert_die_if(gpio >= MAX_POLL_NUM, "gpio%d disable irq , number exceeds the threshold\n", gpio);

    if(pin[gpio].irq_mode  != GPIO_NONE) {
        gpio_enable_irq(&pin[gpio], GPIO_NONE);
        epoll_ctl(epollfd, EPOLL_CTL_DEL, pin[gpio].fd, NULL);
    }
}

static int32_t open_gpio(uint32_t gpio) {
    assert_die_if(gpio >= MAX_POLL_NUM, "gpio%d open , number exceeds the threshold\n", gpio);

    if(pin[gpio].valid == GPIO_VALID)
       return 0;

    if(gpio_open(&pin[gpio], gpio) < 0) {
       LOGE("Failed to gpio%d open\n", gpio);
       return -1;
    }

   return 0;
}

static void close_gpio(uint32_t gpio) {
    assert_die_if(gpio >= MAX_POLL_NUM, "gpio%d close , number exceeds the threshold\n", gpio);

    if(pin[gpio].irq_mode  != GPIO_NONE)
        disable_irq(gpio);

    if(pin[gpio].valid == GPIO_VALID)
        gpio_close(&pin[gpio]);
}

static int32_t set_direction(uint32_t gpio, gpio_direction dir) {
    assert_die_if(gpio >= MAX_POLL_NUM, "gpio%d set direction , number exceeds the threshold\n", gpio);

    if(pin[gpio].valid == GPIO_INVALID) {
        if(gpio_open(&pin[gpio], gpio) < 0)
            goto set_dir_err;
    }

    if(dir == GPIO_OUT) {
       if(gpio_out(&pin[gpio]) < 0)
           goto set_dir_err;
    } else if(dir == GPIO_IN) {
       if(gpio_in(&pin[gpio]) < 0)
           goto set_dir_err;
    }

    return 0;

set_dir_err:
    LOGE("Failed to gpio%d set direction\n", gpio);
    return -1;
}

static int32_t get_direction(uint32_t gpio, gpio_direction *dir) {
    assert_die_if(dir == NULL, "dir is null\n");
    assert_die_if(gpio >= MAX_POLL_NUM, "gpio%d get direction , number exceeds the threshold\n", gpio);

    if(pin[gpio].valid == GPIO_INVALID) {
        if(gpio_open(&pin[gpio], gpio) < 0) {
            LOGE("Failed to gpio%d get direction\n", gpio);
            return -1;
        }
    }

    *dir = pin[gpio].direction;
    return 0;
}

static int32_t get_value(uint32_t gpio, gpio_value *value) {
    assert_die_if(value == NULL, "gpio%d get value , value is null\n", gpio);
    assert_die_if(gpio >= MAX_POLL_NUM, "gpio%d get value , number exceeds the threshold\n", gpio);

    if(pin[gpio].valid == GPIO_INVALID) {
        if(gpio_open(&pin[gpio], gpio) < 0)
            goto get_value_err;
    }

    if(gpio_get_value(&pin[gpio], value) < 0)
        goto get_value_err;

    return 0;

get_value_err:
    LOGE("Failed to gpio%d get value\n", gpio);
    return -1;
}

static int32_t set_value(uint32_t gpio, gpio_value value) {
    assert_die_if(gpio >= MAX_POLL_NUM, "gpio%d set value , number exceeds the threshold\n");

    if(pin[gpio].valid == GPIO_INVALID) {
        if(gpio_open(&pin[gpio], gpio) < 0)
            goto set_value_err;
    }

    if(gpio_set_value(&pin[gpio], value) < 0)
        goto set_value_err;

    return 0;

set_value_err:
    LOGE("Failed to gpio%d set value\n", gpio);
    return -1;
}

static void deinit(void) {
    int data, i;

    for(i = 0; i < MAX_POLL_NUM; i++) {
        if(pin[i].valid == GPIO_VALID)
            close_gpio(i);
    }

    if(init_flag == true) {
        pthread_cancel(irq_pthread_id);
        pthread_join(irq_pthread_id, (void **)&data);
        close(epollfd);
        epollfd = -1;
        init_flag = false;
    }

}

static struct gpio_manager gpio_manager = {
    .init   = init,
    .deinit = deinit,
    .open = open_gpio,
    .close = close_gpio,
    .set_direction = set_direction,
    .get_direction = get_direction,
    .get_value = get_value,
    .set_value = set_value,
    .set_irq_func = set_irq_func,
    .enable_irq = enable_irq,
    .disable_irq = disable_irq,
};

struct gpio_manager* get_gpio_manager(void) {
    return &gpio_manager;
}
