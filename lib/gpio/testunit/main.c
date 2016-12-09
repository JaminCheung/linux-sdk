#include <signal.h>
#include <stdlib.h>
#include <utils/log.h>
#include <lib/gpio/libgpio.h>

#define LOG_TAG "test_libgpio"

#define GPIO_PA(n)  (0*32 + n)
#define GPIO_PB(n)  (1*32 + n)
#define GPIO_PC(n)  (2*32 + n)
#define GPIO_PD(n)  (3*32 + n)

#define LED_GPIO    GPIO_PA(9)
#define KEY_GPIO    GPIO_PA(10)

struct gpio_pin key_pin;
struct gpio_pin led_pin;

static void signal_handler(int signum) {
    gpio_close(&key_pin);
    gpio_close(&led_pin);

    abort();
}

static void register_signal_handler() {
    struct sigaction action, old_action;

    action.sa_handler = signal_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    sigaction(SIGINT, NULL, &old_action);
    if (old_action.sa_handler != SIG_IGN)
        sigaction(SIGINT, &action, NULL);
}

static void msleep(long long msec) {
    struct timespec ts;
    int err;

    ts.tv_sec = (msec / 1000);
    ts.tv_nsec = (msec % 1000) * 1000 * 1000;

    do {
        err = nanosleep(&ts, &ts);
    } while (err < 0 && errno == EINTR);
}

int main(void) {
    gpio_value value;

    if (gpio_open_dir(&key_pin, KEY_GPIO, GPIO_IN) < 0) {
        LOGE("Failed to open key pin");
        goto out;
    }

    if (gpio_open_dir(&led_pin, LED_GPIO, GPIO_OUT) < 0) {
        LOGE("Failed to open led pin");
        goto out;
    }

    if (gpio_enable_irq(&key_pin, GPIO_BOTH) < 0) {
        LOGE("Failed to enable key irq");
        goto out;
    }

    register_signal_handler();

    for (;;) {

        msleep(150);

        if (gpio_irq_wait(&key_pin, &value) < 0) {
            LOGE("Failed to get key pin value");
            goto out;
        }

        if (value == GPIO_LOW)
            gpio_set_value(&led_pin, value);
        else
            gpio_set_value(&led_pin, value);
    }

out:
    gpio_close(&key_pin);
    gpio_close(&led_pin);

    return 0;
}
