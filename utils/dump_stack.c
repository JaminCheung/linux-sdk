/*
 *  Copyright (C) 2017, Zhang YanMing <jamincheung@126.com>
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

#include <stdlib.h>
#include <string.h>
#include <execinfo.h>

#include <types.h>
#include <utils/log.h>

#define LOG_TAG "dump_stack"

void dump_stack(void) {
    void *buffer[128] = {0};
    uint32_t size = 0;
    char** strings = NULL;

    size = backtrace(buffer, 128);
    strings = backtrace_symbols(buffer, size);
    if (strings == NULL) {
        LOGE("Failed to get backtrace symbol: %s\n", strerror(errno));
        return;
    }

    LOGI("========================================\n");
    LOGI("Call trace:\n");
    for (int i = 0; i < size; i++)
        LOGI("%s\n", strings[i]);
    LOGI("========================================\n");

    free(strings);
    strings = NULL;
}
