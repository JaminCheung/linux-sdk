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
#include <uart/uart_manager.h>

#define LOG_TAG "test_uart"

#define PORT_S1_DEVNAME     "/dev/ttyS1"
#define PORT_S1_BAUDRATE    115200
#define PORT_S1_DATABITS      8
#define PORT_S1_PRARITY         UART_PARITY_NONE
#define PORT_S1_STOPBITS       1
#define PORT_S1_TRANSMIT_TIMEOUT   300  //ms
#define PORT_S1_TRANSMIT_LENGTH     20
int main(int argc, char *argv[]) {

    struct uart_manager* uart = get_uart_manager();
    int int_ret;
    unsigned int uint_ret;
    char write_buf[] = "This is write test...\n\r";
    char read_buf[2048];

    if (uart == NULL){
        LOGE("fatal error on getting single instance uart\n");
        return -1;
    }

    int_ret = (int)uart->init(PORT_S1_DEVNAME, PORT_S1_BAUDRATE,
            PORT_S1_DATABITS, PORT_S1_PRARITY, PORT_S1_STOPBITS);
    if (int_ret < 0) {
        LOGE("uart init called failed\n");
        return -1;
    }

    /* This is not essential,  flow control is disabled on default */
    int_ret = uart->flow_control(PORT_S1_DEVNAME, UART_FLOWCONTROL_NONE);
    if (int_ret < 0) {
        LOGE("uart flow control called failed\n");
        return -1;
    }


    uint_ret = uart->write(PORT_S1_DEVNAME,
            write_buf, sizeof(write_buf), PORT_S1_TRANSMIT_TIMEOUT);
    if (uint_ret < sizeof(write_buf)) {
        LOGE("uart write called failed\n");
        return -1;
    }

    while(1) {
        uint_ret = uart->read(PORT_S1_DEVNAME,
            read_buf, PORT_S1_TRANSMIT_LENGTH, PORT_S1_TRANSMIT_TIMEOUT);
        if (uint_ret < PORT_S1_TRANSMIT_LENGTH) {
            LOGE("uart read called failed\n");
            return -1;
        }
        LOGI("gotten: %s\n", read_buf);
        break;
    }
    uart->deinit(PORT_S1_DEVNAME);
    return 0;
}