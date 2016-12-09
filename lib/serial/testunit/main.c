#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
# include <unistd.h>
# include <utime.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <types.h>
#include <utils/log.h>
#include <lib/serial/libserialport_config.h>
#include <lib/serial/libserialport.h>
#include <lib/serial/libserialport_internal.h>

#define LOG_TAG "test_libserialport"

#define SERIAL_PORT "/dev/ttyS1"
#define BAUDRATE 115200

static const char write_buf[] = "This is write test...\n\r";

int main() {
    struct sp_port* port;
    struct sp_port_config *config;

    port = (struct sp_port* ) malloc(sizeof(struct sp_port));

    if (sp_new_config(&config) < 0) {
        LOGE("Failed to create new config\n");
        return -1;
    }

    if (sp_get_port_by_name(SERIAL_PORT, &port) < 0) {
        LOGE("Failed to get port\n");
        return -1;
    }

    if (sp_open(port, SP_MODE_READ_WRITE) < 0) {
        LOGE("Failed to open\n");
        return -1;
    }

    if (sp_get_config(port, config) < 0) {
        LOGE("Failed to get current port config\n");
        return -1;
    }

    if (sp_set_config_bits(config, 8) < 0) {
        LOGE("Failed to set data bits\n");
        return -1;
    }

    if (sp_set_parity(port, SP_PARITY_NONE) < 0) {
        LOGE("Failed to set parity\n");
        return -1;
    }

    if (sp_set_stopbits(port, 1) < 0) {
        LOGE("Failed to set stop bits\n");
        return -1;
    }

    if (sp_set_baudrate(port, BAUDRATE) < 0) {
        LOGE("Failed to set baudrate\n");
        return -1;
    }

    sp_free_config(config);

    if (sp_blocking_write(port, write_buf, sizeof(write_buf), 300) < 0) {
        LOGE("Failed to write\n");
        return -1;
    }

    if (sp_drain(port) < 0) {
        LOGE("Failed to wait out buffer drain\n");
        return -1;
    }

    while(1) {
        uint32_t num = 0;

        num = sp_input_waiting(port);
        if (num < 0) {
            LOGE("Failed to wait input\n");
            return -1;
        } else if (num == 0)
            continue;

        char *buf = malloc(num + 1);
        memset(buf, 0, sizeof(buf));

        if (sp_blocking_read(port, buf, num, 300) < 0) {
            LOGE("Failed to read\n");
            return -1;
        }

        LOGI("Receive: %s\n", buf);
    }


    if (sp_close(port) < 0) {
        LOGE("Failed to close\n");
        return -1;
    }

    return 0;
}
