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
static uint32_t init_count;
static struct battery_manager this;

struct listener {
    battery_event_listener_t cb;
    struct list_head head;
};

static char *health_strs[] = {
        "Unknown", "Good", "Overheat", "Dead", "Over voltage",
        "Unspecified failure", "Cold", "Watchdog timer expire",
        "Safety timer expire"
};
static char *technology_strs[] = {
        "Unknown", "NiMH", "Li-ion", "Li-poly", "LiFe", "NiCd",
        "LiMn"
};

static char *status_strs[] = {
        "Unknown", "Charging", "Discharging", "Not charging", "Full"
};

static void handle_power_supply_event(struct netlink_handler* nh,
        struct netlink_event* event) {
    const int action = event->get_action(event);
    const char* name = event->find_param(event, "POWER_SUPPLY_NAME");

    if (strcmp(name, "battery"))
        return;

    const char* state = event->find_param(event, "POWER_SUPPLY_STATUS");
    const char* cap = event->find_param(event, "POWER_SUPPLY_CAPACITY");
    const char* vol_now = event->find_param(event, "POWER_SUPPLY_VOLTAGE_NOW");
    const char* vol_max = event->find_param(event, "POWER_SUPPLY_VOLTAGE_MAX_DESIGN");
    const char* vol_min = event->find_param(event, "POWER_SUPPLY_VOLTAGE_MIN_DESIGN");
    const char* tech = event->find_param(event, "POWER_SUPPLY_TECHNOLOGY");
    const char* health = event->find_param(event, "POWER_SUPPLY_HEALTH");
    const char* present = event->find_param(event, "POWER_SUPPLY_PRESENT");

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

    if (!strcmp(tech, "NiMH"))
        battery_event->technology = POWER_SUPPLY_TECHNOLOGY_NiMH;
    else if (!strcmp(tech, "Li-ion"))
        battery_event->technology = POWER_SUPPLY_TECHNOLOGY_LION;
    else if (!strcmp(tech, "Li-poly"))
        battery_event->technology = POWER_SUPPLY_TECHNOLOGY_LIPO;
    else if (!strcmp(tech, "LiFe"))
        battery_event->technology = POWER_SUPPLY_TECHNOLOGY_LiFe;
    else if (!strcmp(tech, "NiCd"))
        battery_event->technology = POWER_SUPPLY_TECHNOLOGY_NiCd;
    else if (!strcmp(tech, "LiMn"))
        battery_event->technology = POWER_SUPPLY_TECHNOLOGY_LiMn;
    else if (!strcmp(tech, "Unknown"))
        battery_event->technology = POWER_SUPPLY_TECHNOLOGY_UNKNOWN;

    if (!strcmp(health, "Good"))
        battery_event->health = POWER_SUPPLY_HEALTH_GOOD;
    else if (!strcmp(health, "Overheat"))
        battery_event->health = POWER_SUPPLY_HEALTH_OVERHEAT;
    else if (!strcmp(health, "Dead"))
        battery_event->health = POWER_SUPPLY_HEALTH_DEAD;
    else if (!strcmp(health, "Over voltage"))
        battery_event->health = POWER_SUPPLY_HEALTH_OVERVOLTAGE;
    else if (!strcmp(health, "Unspecified failure"))
        battery_event->health = POWER_SUPPLY_HEALTH_UNSPEC_FAILURE;
    else if (!strcmp(health, "Cold"))
        battery_event->health = POWER_SUPPLY_HEALTH_COLD;
    else if (!strcmp(health, "Watchdog timer expire"))
        battery_event->health = POWER_SUPPLY_HEALTH_WATCHDOG_TIMER_EXPIRE;
    else if (!strcmp(health, "Safety timer expire"))
        battery_event->health = POWER_SUPPLY_HEALTH_SAFETY_TIMER_EXPIRE;
    else if (!strcmp(health, "Unknown"))
        battery_event->health = POWER_SUPPLY_HEALTH_UNKNOWN;

    battery_event->capacity = strtol(cap, NULL, 0);
    battery_event->voltage_now = strtol(vol_now, NULL, 0);
    battery_event->voltage_max = strtol(vol_max, NULL, 0);
    battery_event->voltage_min = strtol(vol_min, NULL, 0);
    battery_event->present = strtol(present, NULL, 0);

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

static void unregister_all_listener(void) {
    struct list_head* pos;
    struct list_head* next_pos;

    pthread_mutex_lock(&listener_lock);

    list_for_each_safe(pos, next_pos, &listeners) {
        struct listener* l = list_entry(pos, struct listener, head);

        list_del(&l->head);
        free(l);
    }

    pthread_mutex_unlock(&listener_lock);
}

static void dump_event(struct battery_event* event) {
    LOGI("========================================\n");
    LOGI("Dump battery event\n");
    LOGI("Technology:  %s\n", technology_strs[event->technology]);
    LOGI("Present:     %u\n", event->present);
    LOGI("Health:      %s\n", health_strs[event->health]);
    LOGI("State:       %s\n", status_strs[event->state]);
    LOGI("Voltage Max: %u\n", event->voltage_max);
    LOGI("Voltage Min: %u\n", event->voltage_min);
    LOGI("Voltage Now: %u\n", event->voltage_now);
    LOGI("Capacity:    %u\n", event->capacity);
    LOGI("========================================\n");
}

static struct netlink_handler* get_netlink_handler(void) {
    return nh;
}

static int init(void) {
    pthread_mutex_lock(&init_lock);

    if (init_count++ == 0) {
        nh = (struct netlink_handler *) calloc(1, sizeof(struct netlink_handler));
        nh->construct = construct_netlink_handler;
        nh->deconstruct = destruct_netlink_handler;
        nh->construct(nh, "all sub-system", 0, handle_event, &this);

        INIT_LIST_HEAD(&listeners);
    }

    pthread_mutex_unlock(&init_lock);

    return 0;
}

static int deinit(void) {
    pthread_mutex_lock(&init_lock);

    if (--init_count == 0) {
        nh->deconstruct(nh);
        free(nh);
        nh = NULL;

        unregister_all_listener();
    }

    pthread_mutex_unlock(&init_lock);

    return 0;
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
