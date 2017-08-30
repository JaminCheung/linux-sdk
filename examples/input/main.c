#include <unistd.h>

#include <utils/log.h>
#include <utils/common.h>
#include <input/input_manager.h>

#define LOG_TAG "test_input"

static struct input_manager *input_manager;

static void input_event_listener(const char* input_name,
        struct input_event* event) {
    LOGI("Input device name: %s\n", input_name);
    input_manager->dump_event(event);
}

int main(int argc, char *argv[]) {
    int error = 0;

    input_manager = get_input_manager();

    error = input_manager->init();
    if (error < 0) {
        LOGE("Failed to init input manager\n");
        return -1;
    }

    error = input_manager->start();
    if (error < 0) {
        LOGE("Failed to start input manager\n");
        return -1;
    }

    input_manager->register_event_listener("gpio-keys", input_event_listener);

    while (1)
        sleep(1000);

    input_manager->unregister_event_listener("gpio-keys", input_event_listener);

    error = input_manager->stop();
    if (error < 0) {
        LOGE("Failed to stop input manager\n");
        return -1;
    }

    error = input_manager->deinit();
    if (error < 0) {
        LOGE("Failed to deinit input manager\n");
        return -1;
    }

    return 0;
}
