/*
 *  Copyright (C) 2017, Zhang YanMing <yanmin.zhang@ingenic.com, jamincheung@126.com>
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
#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <utils/log.h>
#include <utils/assert.h>
#include <netlink/netlink_event.h>

#include "netlink_listener.h"

#define LOG_TAG "netlink_event"

static const char*
has_prefix(const char* str, const char* end, const char* prefix,
        size_t prefixlen) {
    if ((end - str) >= (ptrdiff_t) prefixlen && !memcmp(str, prefix, prefixlen))
        return str + prefixlen;
    else
        return NULL;
}

static int parseBinaryNetlinkMessage(struct netlink_event* this, char *buffer,
        int size) {
    return -1;
}

#define CONST_STRLEN(x)  (sizeof(x)-1)

#define HAS_CONST_PREFIX(str,end,prefix)  has_prefix((str),(end),prefix,CONST_STRLEN(prefix))

static int parseAsciiNetlinkMessage(struct netlink_event* this, char *buffer,
        int size) {
    const char *s = buffer;
    const char *end;
    int param_idx = 0;
    int first = 1;

    if (size == 0)
        return -1;

    buffer[size - 1] = '\0';

    end = s + size;
    while (s < end) {
        if (first) {
            // parse event path
            const char *p = s;
            for (; *p != '@'; p++)
                continue;
            p++;
            this->path = strdup(p + 1);
            first = 0;
        } else {
            const char* a;
            if ((a = HAS_CONST_PREFIX(s, end, "ACTION=")) != NULL) {
                if (!strcmp(a, "add"))
                    this->action = NLACTION_ADD;
                else if (!strcmp(a, "remove"))
                    this->action = NLACTION_REMOVE;
                else if (!strcmp(a, "change"))
                    this->action = NLACTION_CHANGE;
            } else if ((a = HAS_CONST_PREFIX(s, end, "SEQNUM=")) != NULL) {
                this->seq = atoi(a);
            } else if ((a = HAS_CONST_PREFIX(s, end, "SUBSYSTEM=")) != NULL) {
                this->subsystem = strdup(a);
            } else if (param_idx < NL_PARAMS_MAX) {
                this->params[param_idx++] = strdup(s);
            }
        }
        s += strlen(s) + 1;
    }

    return 0;
}

static int decode(struct netlink_event *this, char *buffer, int size,
        int format) {
    if (format == NETLINK_FORMAT_BINARY) {
        return parseBinaryNetlinkMessage(this, buffer, size);
    } else {
        return parseAsciiNetlinkMessage(this, buffer, size);
    }
}

static const char *find_param(struct netlink_event* this,
        const char* param_name) {
    int i;
    size_t len = strlen(param_name);
    for (i = 0; i < NL_PARAMS_MAX && this->params[i] != NULL; ++i) {
        const char *ptr = this->params[i] + len;
        if (!strncmp(this->params[i], param_name, len) && *ptr == '=')
            return ++ptr;
    }

    LOGD("Parameter '%s' not found\n", param_name);

    return NULL;
}

static const char* get_subsystem(struct netlink_event* this) {
    return this->subsystem;
}

static const int get_action(struct netlink_event* this) {
    return this->action;
}

static void dump(struct netlink_event* this) {
    static const char *action[] = { "", "add", "remove", "change" };
    int i;

    LOGD("========================================\n");
    LOGD("Dump uevent\n");
    LOGD("NL seqnum \'%d\'\n", this->seq);
    LOGD("NL subsytem \'%s\'\n", this->subsystem);
    LOGD("NL devpath \'%s\'\n", this->path);
    LOGD("NL action \'%s\'\n", action[this->action]);
    for (i = 0; i < NL_PARAMS_MAX; i++) {
        if (!this->params[i])
            break;
        LOGD("NL param \'%s\'\n", this->params[i]);
    }
    LOGD("========================================\n");
}

void construct_netlink_event(struct netlink_event* this) {
    this->path = NULL;
    this->action = NLACTION_UNKNOWN;
    this->subsystem = NULL;
    this->seq = 0;
    memset(this->params, 0, sizeof(this->params));

    this->decode = decode;
    this->find_param = find_param;
    this->get_subsystem = get_subsystem;
    this->get_action = get_action;
    this->dump = dump;
}

void destruct_netlink_event(struct netlink_event* this) {

    if (this->path)
        free(this->path);

    if (this->subsystem)
        free(this->subsystem);

    int i;
    for (i = 0; i < NL_PARAMS_MAX; i++) {
        if (!this->params[i])
            free(this->params[i]);
    }
}
