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

#include <string.h>
#include <stdlib.h>

#include <utils/log.h>
#include <utils/list.h>
#include <utils/assert.h>
#include <netlink/netlink_handler.h>
#include <netlink/netlink_event.h>
#include <battery/battery_manager.h>

#define LOG_TAG "battery_manager"

static struct list_head listeners;
static pthread_mutex_t listener_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t init_lock = PTHREAD_MUTEX_INITIALIZER;
static struct netlink_handler* nh;
static int inited;
static struct battery_manager this;

struct listener {
    battery_event_listener_t cb;
    struct list_head head;
};

static const char* state2str(uint8_t state) {
    switch (state) {
    case POWER_SUPPLY_STATUS_CHARGING:
        return "Charging";
    case POWER_SUPPLY_STATUS_DISCHARGING:
        return "Discharging";
    case POWER_SUPPLY_STATUS_NOT_CHARGING:
        return "Charging";
    case POWER_SUPPLY_STATUS_FULL:
        return "Full";
    default:
        return "Unknown";
    }
}

static void handle_power_supply_event(struct netlink_handler* nh,
        struct netlink_event* event) {
    const int action = event->get_action(event);
    const char* name = event->find_param(event, "POWER_SUPPLY_NAME");

    if (strcmp(name, "battery"))
        return;

    const char* state = event->find_param(event, "POWER_SUPPLY_STATUS");
    const char* cap = event->find_param(event, "POWER_SUPPLY_CAPACITY");

    struct battery_event *battery_event = calloc(1,
            sizeof(struct battery_event));

    if (action == NLACTION_CHANGE || action == NLACTION_ADD) {
        if (!strcmp(state, "Charging"))
            battery_event->state = POWER_SUPPLY_STATUS_CHARGING;
        else if (!strcmp(state, "Not charging"))
            battery_event->state = POWER_SUPPLY_STATUS_NOT_CHARGING;
        else if (!strcmp(state, "Discharging"))
            battery_event->state = POWER_SUPPLY_STATUS_DISCHARGING;
        else if (!strcmp(state, "Full"))
            battery_event->state = POWER_SUPPLY_STATUS_FULL;
        else if (!strcmp(state, "Unknown"))
            battery_event->state = POWER_SUPPLY_STATUS_UNKNOWN;
    }

    battery_event->capacity = strtol(cap, NULL, 0);

    pthread_mutex_lock(&listener_lock);

    struct list_head* pos;
    list_for_each(pos, &listeners) {
        struct listener* l = list_entry(pos, struct listener, head);
        l->cb(battery_event);
    }

    pthread_mutex_unlock(&listener_lock);

    free(battery_event);
}

static void handle_event(struct netlink_handler* nh,
        struct netlink_event* event) {
    const char* subsystem = event->get_subsystem(event);

    if (!strcmp(subsystem, "power_supply"))
        event->dump(event);

    if (!strcmp(subsystem, "power_supply"))
        handle_power_supply_event(nh, event);
}

static int init(void) {
    pthread_mutex_lock(&init_lock);
    if (inited == 1) {
        LOGE("battery manager already init\n");
        pthread_mutex_unlock(&init_lock);
        return -1;
    }

    nh = (struct netlink_handler *) calloc(1, sizeof(struct netlink_handler));
    nh->construct = construct_netlink_handler;
    nh->deconstruct = destruct_netlink_handler;
    nh->construct(nh, "all sub-system", 0, handle_event, &this);

    INIT_LIST_HEAD(&listeners);
    inited = 1;

    pthread_mutex_unlock(&init_lock);

    return 0;
}

static int deinit(void) {
    pthread_mutex_lock(&init_lock);

    if (inited == 0) {
        LOGE("battery manager already deinit\n");
        pthread_mutex_unlock(&init_lock);
        return -1;
    }
    inited = 0;

    nh->deconstruct(nh);
    free(nh);
    nh = NULL;

    pthread_mutex_unlock(&init_lock);

    return 0;
}

static void register_event_listener(battery_event_listener_t listener) {
    assert_die_if(listener == NULL, "listener is NULL\n");

    struct list_head* pos;
    pthread_mutex_lock(&listener_lock);

    list_for_each(pos, &listeners) {
        struct listener *l = list_entry(pos, struct listener, head);
        if (l->cb == listener) {
            pthread_mutex_unlock(&listener_lock);
            return;
        }
    }

    struct listener *l = malloc(sizeof(struct listener));
    l->cb = listener;
    list_add_tail(&l->head, &listeners);

    pthread_mutex_unlock(&listener_lock);
}

static void unregister_event_listener(battery_event_listener_t listener) {
    assert_die_if(listener == NULL, "listener is NULL\n");

    struct list_head* pos;
    struct list_head* next_pos;

    pthread_mutex_lock(&listener_lock);

    list_for_each_safe(pos, next_pos, &listeners) {
        struct listener* l = list_entry(pos, struct listener, head);

        if (l->cb == listener) {
            list_del(&l->head);

            free(l);

            pthread_mutex_unlock(&listener_lock);

            return;
        }
    }

    pthread_mutex_unlock(&listener_lock);
}

static void dump_event(struct battery_event* event) {
    LOGI("========================================\n");
    LOGI("Dump battery event\n");
    LOGI("State: %s\n", state2str(event->state));
    LOGI("Capacity: %u\n", event->capacity);
    LOGI("========================================\n");
}

static struct netlink_handler* get_netlink_handler(void) {
    return nh;
}

static struct battery_manager this = {
        .init = init,
        .deinit = deinit,
        .register_event_listener = register_event_listener,
        .unregister_event_listener = unregister_event_listener,
        .get_netlink_handler = get_netlink_handler,
        .dump_event = dump_event,
};

struct battery_manager* get_battery_manager(void) {
    return &this;
}
