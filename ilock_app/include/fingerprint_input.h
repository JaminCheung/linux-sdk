#ifndef _FINGERPRINT_INPUT_H_
#define _FINGERPRINT_INPUT_H_

#include <ma_fingerprint.h>

struct fingerprint_input {
    int (*init)(void);
    int (*deinit)(void);
    void (*register_event_listener)(int listener);
    void (*unregister_event_listener)(int listener);
    int (*enroll)(void);
    int (*authenticate)(void);
    int (*delete)(int finger_id, int type);
    int (*cancel)(void);
    int (*reset)(void);
    int (*set_enroll_timeout)(int timeout_ms);
    int (*set_authenticate_timeout)(int timeout_ms);
    int (*post_power_on)(void);
    int (*post_power_off)(void);
};

struct input_manager* get_fingerprint_input(void);

#endif
