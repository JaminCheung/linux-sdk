#ifndef _INPUT_MANAGER_H_
#define _INPUT_MANAGER_H_

#include <keyboard_input.h>
#include <fingerprint_input.h>

typedef struct input_manager *   input_mode_device_t;

enum work_mode {
    NORMAL_MODE                         = 1,
    ADMIN_MODE                          = 2,
};

enum unlock_mode {
    UNLOCK_SECURITY_KEY                 = 1,
    UNLOCK_PASSWORD                     = 2,
    UNLOCK_RFID                         = 3,
    UNLOCK_FINGERPRINT                  = 4,
    UNLOCK_FACE_ID                      = 5,
    UNLOCK_BLUETOOTH                    = 6,
};


enum error_result {
    SECURITY_KEY_AUTH_FAILED            = -6,
    PASSWORD_AUTH_FAILED                = -5,
    RFID_AUTH_FAILED                    = -4,
    FINGERPRINT_AUTH_FAILED             = -3,
    FACE_ID_AUTH_FAILED                 = -2,
    BLUETOOTH_AUTH_FAILED               = -1,

    AUTH_SUCCESS                        = 0,
    AUTH_ALERT_SUCCESS,
    DOOR_UNLOCK_SUCCESS,

    HINT_NORMAL_WAKEUP                  = 3,

    ADD_NEW_ELEMENT,
    DELETE_ELEMENT,
};

struct input_dev_param {
    int key_code;
    int key_value;
    int input_mode;
    int enroll_result;
    int auth_result;
    int del_result;
    int delete_id;
    int delete_mode;
    char content[32];

    pthread_mutex_t queue_lock;
    pthread_cond_t queue_ready;
};

struct input_manager {
    char *name[128];
    struct list_head node;
    input_mode_device_t cb;

    int (*init)(void);
    int (*deinit)(void);
    void (*register_event_listener)(int listener);
    void (*unregister_event_listener)(int listener);
    int (*enroll)(struct input_dev_param *param);
    int (*authenticate)(struct input_dev_param *param);
    int (*delete)(struct input_dev_param *param);
    int (*cancel)(void);
    int (*reset)(void);
    int (*set_enroll_timeout)(int timeout_ms);
    int (*set_authenticate_timeout)(int timeout_ms);
    int (*post_power_on)(void);
    int (*post_power_off)(void);
    int (*work_mode)(struct input_dev_param *param);
    int (*get_keycode)(struct input_dev_param *param);

    pthread_mutex_t lock;
};

struct input_manager* get_input_manager(void);

#endif
