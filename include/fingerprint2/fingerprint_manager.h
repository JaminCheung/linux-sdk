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

#ifndef FINGERPRINT_MANAGER_H
#define FINGERPRINT_MANAGER_H

typedef enum {
    FP_MSG_FINGER_DOWN          = 0,
    FP_MSG_FINGER_UP            = 1,
    FP_MSG_NO_LIVE              = 2,
    FP_MSG_PROCESS_SUCCESS      = 3,
    FP_MSG_PROCESS_FAILED       = 4,
    FP_MSG_ENROLL_SUCCESS       = 5,
    FP_MSG_ENROLL_FAILED        = 6,
    FP_MSG_MATCH_SUCESS         = 7,
    FP_MSG_MATCH_FAILED         = 8,
    FP_MSG_CANCEL               = 9,
    FP_MSG_TIMEOUT              = 10,
    FP_MSG_MAX
}fp_msg_t;

typedef void (*fingerprint_event_listener_t)(int msg, int percent, int finger_id);

typedef enum {
    DELETE_BY_ID,
    DELETE_ALL,
} delete_type_t;

struct fingerprint_manager {
    int (*init)(void);
    int (*deinit)(void);
    void (*register_event_listener)(fingerprint_event_listener_t listener);
    void (*unregister_event_listener)(fingerprint_event_listener_t listener);
    int (*enroll)(void);
    int (*authenticate)(void);
    int (*delete)(int finger_id, delete_type_t type);
    int (*cancel)(void);
    int (*reset)(void);
    int (*set_enroll_timeout)(int timeout_ms);
    int (*set_authenticate_timeout)(int timeout_ms);
    int (*post_power_on)(void);
    int (*post_power_off)(void);
};

struct fingerprint_manager* get_fingerprint_manager(void);

#endif /* FINGERPRINT_MANAGER_H */
