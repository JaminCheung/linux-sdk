#ifndef _DOOR_MANAGER_H_
#define _DOOR_MANAGER_H_


enum door_state {
    DOOR_LOCKED                         = 0,
    DOOR_UNLOCKING                      = 1,
    DOOR_UNLOCKED                       = 2,


};

typedef void (*door_state_listener_t)(int door_state);

struct door_manager {
    int (*init)(void);
    int (*deinit)(void);
    void (*register_door_listener)(door_state_listener_t listener);
    void (*unregister_door_listener)(door_state_listener_t listener);
    int (*ctrl_door_state)(int state);
    int (*get_door_state)(void);
};

struct door_manager* get_door_manager(void);

#endif
