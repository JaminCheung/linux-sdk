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

#include <stdbool.h>
#include <stdarg.h>
#include <stdlib.h>

#include <utils/log.h>
#include <utils/common.h>

#define LOG_TAG "assert"

#define BUF_SIZE    (1024 * 1)

void assert_die_if(bool condition, const char* fmt, ...) {
    if (!condition)
        return;

    va_list ap;
    char buf[BUF_SIZE] = { 0 };

    va_start(ap, fmt);
    vsnprintf(buf, BUF_SIZE, fmt, ap);
    va_end(ap);

    LOGE("============ Assert Failed ============\n");
    LOGE("Message: %s", buf);
    LOGE("========== Assert Failed End ==========\n");
    dump_stack();

    exit(-1);
}
