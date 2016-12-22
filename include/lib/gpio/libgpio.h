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

#ifndef LIB_GPIO_H
#define LIB_GPIO_H

#include <types.h>
#include <libqrcode_api.h>
#ifdef __cplusplus
extern "C" {
#endif
int gpio_open(struct gpio_pin *pin, unsigned int no);
int gpio_open_by_name(struct gpio_pin *pin, const char *name);
int gpio_open_dir(struct gpio_pin *pin, unsigned int no, gpio_direction dir);
int gpio_open_by_name_dir(struct gpio_pin *pin, const char *name, gpio_direction dir);

int gpio_close(struct gpio_pin *pin);
void gpio_destroy(void);

int gpio_out(struct gpio_pin *pin);
int gpio_in(struct gpio_pin *pin);

int gpio_set_value(struct gpio_pin *pin, gpio_value value);
int gpio_get_value(struct gpio_pin *pin, gpio_value *value);

int gpio_enable_irq(struct gpio_pin *pin, gpio_irq_mode m);
int gpio_irq_wait(struct gpio_pin *pin, gpio_value *value);
int gpio_irq_timed_wait(struct gpio_pin *pin, gpio_value *value, int timeout_ms);

int gpio_get_fd(struct gpio_pin *pin);

#ifdef __cplusplus
}
#endif

#endif /* LIB_GPIO_H */
