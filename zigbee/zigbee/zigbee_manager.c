/*
 *  Copyright (C) 2017, Monk Su<rongjin.su@ingenic.com, MonkSu@outlook.com>
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <utils/log.h>
#include <uart/uart_manager.h>
#include <zigbee/zigbee/zigbee_manager.h>
#include <zigbee/protocol/uart_protocol.h>
#include <timer/timer_manager.h>

/*
 * ioctl commands
 */

#define LOG_TAG  "zigbee_manager"

#define ZB_CC2530_IOC_MAGIC             'Z'
#define ZB_CC2530_IOC_POWER             _IOW(ZB_CC2530_IOC_MAGIC, 1, int)   /* set power */
#define ZB_CC2530_IOC_RST               _IOW(ZB_CC2530_IOC_MAGIC, 2, int)   /* send reset sig to 2530 by io rst_pin */
#define ZB_CC2530_IOC_WAKE              _IOW(ZB_CC2530_IOC_MAGIC, 3, int)   /* send wake up sig to 2530 by io wake_pin */

#define ZIGBEE_DEV_NAME                 "/dev/zigbee"
#define POWER_ON                        1
#define POWER_OFF                       0
#define INTERRUPT_ENABLE                1
#define INTERRUPT_DISABLE               0


#define PORT_S1_DEVNAME                 "/dev/ttyS1"
#define PORT_S1_BAUDRATE                115200
#define PORT_S1_DATABITS                8
#define PORT_S1_PRARITY                 0
#define PORT_S1_STOPBITS                1
#define PORT_S1_TRANSMIT_TIMEOUT        300  //ms
#define PORT_S1_TRANSMIT_LENGTH         10

#define TIMER_ID                        1
#define TIMER_PERIOD                    1000

#define ZB_CFG_RESET                    0x00
#define ZB_CFG_REBOOT                   0x01
#define ZB_CFG_ROLE                     0x02
#define ZB_CFG_PID                      0x03
#define ZB_CFG_CHANNEL                  0x04
#define ZB_CFG_SECKEY                   0x05
#define ZB_CFG_JOIN                     0x06
#define ZB_CFG_CASETYPE                 0x07
#define ZB_CFG_GROUPID                  0x08
#define ZB_CFG_POLL_RATE                0x09
#define ZB_CFG_TX_POWER                 0x0A
#define ZB_CFG_GET_INFO                 0x0B


#define UART_CMD_DATA_REQUEST           0x81
#define UART_CMD_DATA_CONFIRM           0x82

#define SEND_SUCCESS                    0
#define SEND_TIMEOUT                    -1

#define READ_BUF_LEN                    UART_PRO_BUF_LEN
#define SEND_BUF_LEN                    UART_PRO_BUF_LEN

#define MKSEND_BUF(buf, cmd, len)            \
        do{                                  \
            memcpy(devc.send_buf,buf,len);   \
            devc.send_cmd = cmd;             \
            devc.send_len = len;             \
        }while(0)


#define CLEAN_SEND_BUF()                devc.send_len = 0
#define SET_SEND_RESULT(x)              devc.send_result = x
#define GET_SEND_RESULT()               devc.send_result

static int16_t zb_uart_send(uint8_t* buf, uint16_t len);
static int16_t zb_data_request(void);



static pro_src_st src = {
    .rbuf_len = UART_RECV_BUF_LEN,
    .pbuf_len = UART_PRO_BUF_LEN,
    .sbuf_len = UART_SEND_BUF_LEN
};


static struct uart_manager* uart;
static struct uart_pro_manager* uart_pro;
static struct timer_manager *timer;


struct dev_ctrl{
    int fd;                             // dev/zigbee  fd
    pthread_t poll_ptid;                // poll receive thread
    pthread_mutex_t init_mutex;         // init mutex lock
    pthread_mutex_t send_mutex;         // send mutex lock
    pthread_cond_t cond;                // send block
    uint8_t read_buf[READ_BUF_LEN];     // read form uart
    uint8_t send_buf[SEND_BUF_LEN];     // hold data of will be request
    uint8_t send_cmd;
    int8_t send_result;
    uint16_t send_len;
    uart_zigbee_recv_cb recv_cb;        // register to protocol receive callback
};


struct dev_ctrl devc = {
    .fd = -1,
    .init_mutex = PTHREAD_MUTEX_INITIALIZER,
    .send_mutex = PTHREAD_MUTEX_INITIALIZER,
    .cond = PTHREAD_COND_INITIALIZER,
    .send_len = 0,
};

/**
 *  @fn      sig_handler
 *
 *  @brief   io driver interrupt sig, wake up to request data
 *
 *  @param   sig: alway SIGIO
 *
 *  @return  none
 */
static void sig_handler(int sig)
{
    if (sig == SIGIO) {
        zb_data_request();
    }
}


/**
 *  @fn      zb_uart_recv_cb
 *
 *  @brief   protocol receive callback
 *
 *  @param   cmd\payload\payload lenth extract from protocol packet.
 *
 *  @return  none
 */
static void zb_uart_recv_cb(uint8_t cmd, uint8_t* const pl, uint16_t len)
{
    int16_t ret = -1;

    if (cmd == UART_CMD_DATA_REQUEST) {
        if (devc.send_len > 0) {
            uart_pro->send(&src, devc.send_cmd, devc.send_buf, devc.send_len, zb_uart_send);
        }
    } else if (cmd == UART_CMD_DATA_CONFIRM) {
        CLEAN_SEND_BUF();
        ret = timer->stop(TIMER_ID);
        if (ret < 0) {
            LOGW("send timeout timer stop fail. err: %d\n", ret);
        }
        pthread_mutex_lock(&devc.send_mutex);
        SET_SEND_RESULT(SEND_SUCCESS);
        pthread_cond_signal(&devc.cond);
        pthread_mutex_unlock(&devc.send_mutex);

    } else {
        if (devc.recv_cb != NULL) {
            ret = uart_pro->send(&src, UART_CMD_DATA_CONFIRM, NULL, 0, zb_uart_send);
            devc.recv_cb(cmd, pl, len);
        }
    }
}



/**
 *  @fn      send_timeout_cb
 *
 *  @brief   timeout of hold data
 *
 *  @param   unused
 *
 *  @return  none
 */
static void send_timeout_cb(void *args)
{
    int ret;
    (void)args;

    ret = timer->stop(TIMER_ID);
    if (ret < 0) {
        LOGE("send timeout timer stop fail. err: %d\n", ret);
    }

    CLEAN_SEND_BUF();
    pthread_mutex_lock(&devc.send_mutex);
    SET_SEND_RESULT(SEND_TIMEOUT);
    pthread_cond_signal(&devc.cond);
    pthread_mutex_unlock(&devc.send_mutex);
}

/**
 *  @fn      zb_poll
 *
 *  @brief   handle uart zigbee data, result to callback function, don't case uart procotol.
 *
 *  @param   unused
 *
 *  @return  none
 */
static void* zb_poll(void* p)
{
    int oldstate;
    int16_t ret;
    (void)p;

    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &oldstate);

    while(1) {
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldstate);
        ret = uart->read(PORT_S1_DEVNAME, devc.read_buf, READ_BUF_LEN, PORT_S1_TRANSMIT_TIMEOUT);
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &oldstate);
        if (ret > 0) {
            uart_pro->receive(&src, devc.read_buf, ret);
            uart_pro->handle(&src, zb_uart_recv_cb);
        }
    }

    return NULL;
}

/**
 *  @fn      zb_data_request
 *
 *  @brief   interrupt wake up and request data
 *
 *
 *  @return  none
 */
static int16_t zb_data_request(void)
{
    int16_t ret;

    ret = uart_pro->send(&src, UART_CMD_DATA_REQUEST, NULL, 0, zb_uart_send);

    return ret;
}

/**
 *  @fn      zb_dev_init
 *
 *  @brief   peripheral initialization zigbee\protocol\uart\timer
 *
 *
 *  @return  none
 */
static int zb_dev_init(void)
{
    int ret;

    devc.fd = open(ZIGBEE_DEV_NAME, O_RDWR);
    if (devc.fd < 0) {
        LOGE("%s dev: %s open fail\n",__FUNCTION__,ZIGBEE_DEV_NAME);
        ret = -1;
        goto err_dev_open_fail;
    }

    uart = get_uart_manager();
    ret = uart->init(PORT_S1_DEVNAME, PORT_S1_BAUDRATE, PORT_S1_DATABITS,
                                      PORT_S1_PRARITY,  PORT_S1_STOPBITS);
    if (ret < 0) {
        LOGE("uart init called failed\n\r");
        ret = -3;
        goto err_uart_init_fail;
    }

    ret = uart->flow_control(PORT_S1_DEVNAME, UART_FLOWCONTROL_NONE);
    if (ret < 0) {
        LOGE("uart flow_control called failed\n\r");
        ret = -3;
        goto err_uart_cfg_fail;
    }
    uart_pro = get_uart_pro_manager();
    ret = uart_pro->init(&src);
    if (ret < 0) {
        LOGE("uart protocol init failed\n\r");
        ret = -4;
        goto err_pro_init_fail;
    }

    timer = get_timer_manager();
    ret = timer->init(TIMER_ID, TIMER_PERIOD, 1, send_timeout_cb, NULL);
    if (ret < 0 || (ret != TIMER_ID)) {
        ret = -5;
        LOGE("timer init fail\n");
        goto err_timer_init_fail;
    }

    return 0;

err_timer_init_fail:
    uart_pro->deinit(&src);
err_pro_init_fail:
err_uart_cfg_fail:
    uart->deinit(PORT_S1_DEVNAME);
err_uart_init_fail:
    close(devc.fd);
err_dev_open_fail:
    return ret;
}
/**
 *  @fn      zb_dev_deinit
 *
 *  @brief   peripheral deinitialization zigbee\protocol\uart\timer
 *
 *
 *  @return  none
 */
static void zb_dev_deinit(void)
{
    timer->deinit(TIMER_ID);
    uart_pro->deinit(&src);
    uart->deinit(PORT_S1_DEVNAME);
    close(devc.fd);
}



/**
 *  @fn      zb_logic_init
 *
 *  @brief   logic initialization signal\thread
 *
 *
 *  @return  none
 */
static int zb_logic_init(uart_zigbee_recv_cb recv_cb)
{
    int ret;

    devc.recv_cb = recv_cb;

    signal(SIGIO, sig_handler);
    fcntl(devc.fd, F_SETOWN, getpid());
    fcntl(devc.fd, F_SETFL, fcntl(devc.fd, F_GETFL) | FASYNC);

    ret = pthread_create(&devc.poll_ptid, NULL, zb_poll, NULL);
    if (ret < 0) {
        LOGE("pthread create fail. err: %d\n",ret);
        return -1;
    }

    return 0;
}
/**
 *  @fn      zb_logic_deinit
 *
 *  @brief   logic deinitialization thread
 *
 *
 *  @return  none
 */
static void zb_logic_deinit(void)
{
    pthread_cancel(devc.poll_ptid);
    pthread_join(devc.poll_ptid, NULL);
}


/**
 *  @fn      zb_init
 *
 *  @brief   zigbee initialization, dev and logic
 *
 *
 *  @return   0   success
 *           -1  illigle enter
 *           -2  repeated initialization
 *           -3  dev init fail
 *           -4  logic init fail
 */
static int zb_init(uart_zigbee_recv_cb recv_cb)
{
    int ret;

    ret = pthread_mutex_trylock(&(devc.init_mutex));
    if (ret != 0) {
        printf(" pthread tyr lock fail ret: %d\n", ret);
        return -2;
    }

    if (recv_cb == NULL) {
        return -1;
    }

    ret = zb_dev_init();
    if (ret < 0) {
        LOGE("zigbee dev init ret: %d\n", ret);
        return -3;
    }

    ret = zb_logic_init(recv_cb);
    if (ret < 0) {
        LOGE("zigbee logic init ret: %d\n", ret);
        zb_dev_deinit();
        return -4;
    }
    return 0;
}


/**
 *  @fn      zb_trigger_send
 *
 *  @brief   make io interrupt to wake up 2530
 *
 *  @note    it may be block in mutex lock
 *
 *  @return  none
 */
static int zb_trigger_send(void)
{
    int ret;

    pthread_mutex_lock(&devc.send_mutex);

    if (devc.fd >= 0) {
        ret = ioctl(devc.fd, ZB_CC2530_IOC_WAKE, 0);
        if (ret < 0) {
            LOGE("zigbee wake up ioctl fail. err: %d\n", ret);
            pthread_mutex_unlock(&devc.send_mutex);
            return ret;
        }

        ret = timer->start(TIMER_ID);
        if (ret < 0) {
            LOGE("send timeout timer start fail. err: %d\n", ret);
            pthread_mutex_unlock(&devc.send_mutex);
            return ret;
        }
    }

    pthread_cond_wait(&devc.cond, &devc.send_mutex);
    pthread_mutex_unlock(&devc.send_mutex);

    return GET_SEND_RESULT();
}


/**
 *  @fn      zb_uart_send
 *
 *  @brief   uart send handle function, income protocol module to adapter platment
 *
 *  @param   buf - data will be send
 *  @param   len - data length
 *
 *  @return  sent data length
 */
static int16_t zb_uart_send(uint8_t* buf, uint16_t len)
{
    int ret;

    if (buf == NULL)
        return -1;

    ret = uart->write(PORT_S1_DEVNAME, buf, len, PORT_S1_TRANSMIT_TIMEOUT);
    if (ret < 0) {
        LOGE("uart write failed\n\r");
    }

    return ret;
}


/**
 *  @fn      zb_ctrl
 *
 *  @brief   application ctrcol data transmit transparently
 *
 *  @param   pl - ctrl data
 *  @param   len - data length
 *
 *  @return  0 success
 *          -1 illegal entry
 */
static int zb_ctrl(uint8_t* pl, uint16_t len)
{
    int ret;

    if (pl == NULL || len == 0 || len > SEND_BUF_LEN) {
        LOGE("zigbee ctrl data invalid\r\n");
        return -1;
    }

    MKSEND_BUF(pl,UART_CMD_CTRL,len);

    ret = zb_trigger_send();
    if (ret < 0) {
        LOGW("%s send timeout\r\n",__FUNCTION__);
        return -2;
    }

    return 0;
}

static int zb_get_info(void)
{
    uint8_t sendbuf[1] = {0};
    int ret = -1;

    sendbuf[0] = ZB_CFG_GET_INFO;

    MKSEND_BUF(sendbuf, UART_CMD_CFG, sizeof(sendbuf));

    ret = zb_trigger_send();
    if (ret < 0) {
        LOGW("%s send timeout\r\n",__FUNCTION__);
        return -1;
    }

    return 0;
}

static int zb_cfg_factory(void)
{
    uint8_t sendbuf[1] = {0};
    int ret = -1;

    sendbuf[0] = ZB_CFG_RESET;

    MKSEND_BUF(sendbuf, UART_CMD_CFG, sizeof(sendbuf));

    ret = zb_trigger_send();
    if (ret < 0) {
        LOGW("%s send timeout\r\n",__FUNCTION__);
        return -1;
    }

    return 0;
}

static int zb_hard_reset(void)
{
    int ret;
    ret = ioctl(devc.fd, ZB_CC2530_IOC_RST, 0);
    if (ret < 0) {
        LOGE("%s dev: %s reset ctrl fail\n",__FUNCTION__,ZIGBEE_DEV_NAME);
        return -1;
    }
    return 0;
}

static int zb_cfg_reboot(void) {
    uint8_t sendbuf[1] = {0};
    int ret = -1;

    sendbuf[0] = ZB_CFG_REBOOT;

    MKSEND_BUF(sendbuf, UART_CMD_CFG, sizeof(sendbuf));

    ret = zb_trigger_send();
    if (ret < 0) {
        LOGW("%s send timeout\r\n",__FUNCTION__);
        return -1;
    }

    return 0;
}

static int zb_cfg_role(uint8_t role)
{
    uint8_t sendbuf[2] = {0};
    int ret = -1;

    if (role > 2)
        return -1;

    sendbuf[0] = ZB_CFG_ROLE;
    sendbuf[1] = role;

    MKSEND_BUF(sendbuf, UART_CMD_CFG, sizeof(sendbuf));

    ret = zb_trigger_send();
    if (ret < 0) {
        LOGW("%s send timeout\r\n",__FUNCTION__);
        return -2;
    }

    return 0;
}

static int zb_cfg_pid(uint16_t panid)
{
    uint8_t sendbuf[3] = {0};
    int ret = -1;

    sendbuf[0] = ZB_CFG_PID;
    sendbuf[1] = (uint8_t)((panid >> 8)&0x00FF);
    sendbuf[2] = (uint8_t)((panid)&0x00FF);


    MKSEND_BUF(sendbuf, UART_CMD_CFG, sizeof(sendbuf));

    ret = zb_trigger_send();
    if (ret < 0) {
        LOGW("%s send timeout\r\n",__FUNCTION__);
        return -1;
    }

    return 0;
}

static int zb_cfg_channel(uint8_t channel)
{
    uint8_t sendbuf[2] = {0};
    int ret = -1;

    if (channel > 0x1A || channel < 0x0B)
        return -1;

    sendbuf[0] = ZB_CFG_CHANNEL;
    sendbuf[1] = channel;

    MKSEND_BUF(sendbuf, UART_CMD_CFG, sizeof(sendbuf));

    ret = zb_trigger_send();
    if (ret < 0) {
        LOGW("%s send timeout\r\n",__FUNCTION__);
        return -2;
    }

    return 0;
}

static int zb_cfg_seckey(uint8_t* key, uint8_t keylen)
{
    uint8_t sendbuf[17] = {0};
    int ret = -1;
    if (key == NULL || keylen != 16)
        return -1;

    sendbuf[0] = ZB_CFG_SECKEY;
    memcpy(&sendbuf[1], key, keylen);

    MKSEND_BUF(sendbuf, UART_CMD_CFG, sizeof(sendbuf));

    ret = zb_trigger_send();
    if (ret < 0) {
        LOGW("%s send timeout\r\n",__FUNCTION__);
        return -2;
    }

    return 0;
}

static int zb_cfg_join_aging(uint8_t aging)
{
    uint8_t sendbuf[2] = {0};
    int ret = -1;

    sendbuf[0] = ZB_CFG_JOIN;
    sendbuf[1] = aging;

    MKSEND_BUF(sendbuf, UART_CMD_CFG, sizeof(sendbuf));

    ret = zb_trigger_send();
    if (ret < 0) {
        LOGW("%s send timeout\r\n",__FUNCTION__);
        return -1;
    }

    return 0;
}

static int zb_cfg_cast_type(uint8_t type, uint16_t addr)
{
    uint8_t sendbuf[4] = {0};
    int ret = -1;

    if (type > 2)
        return -1;

    sendbuf[0] = ZB_CFG_CASETYPE;
    sendbuf[1] = type;
    sendbuf[2] = (uint8_t)((addr>>8)&0x00FF);
    sendbuf[3] = (uint8_t)(addr&0x00FF);

    MKSEND_BUF(sendbuf, UART_CMD_CFG, sizeof(sendbuf));

    ret = zb_trigger_send();
    if (ret < 0) {
        LOGW("%s send timeout\r\n",__FUNCTION__);
        return -2;
    }

    return 0;
}

static int zb_cfg_groupid(uint16_t id)
{
    uint8_t sendbuf[3] = {0};
    int ret = -1;

    sendbuf[0] = ZB_CFG_GROUPID;
    sendbuf[1] = (uint8_t)((id>>8)&0x00FF);
    sendbuf[2] = (uint8_t)(id&0x00FF);

    MKSEND_BUF(sendbuf, UART_CMD_CFG, sizeof(sendbuf));

    ret = zb_trigger_send();
    if (ret < 0) {
        LOGW("%s send timeout\r\n",__FUNCTION__);
        return -1;
    }

    return 0;
}

static int zb_cfg_poll_rate(uint16_t rate)
{
    uint8_t sendbuf[3] = {0};
    int ret = -1;

    sendbuf[0] = ZB_CFG_POLL_RATE;
    sendbuf[1] = (uint8_t)((rate>>8)&0x00FF);
    sendbuf[2] = (uint8_t)(rate&0x00FF);

    MKSEND_BUF(sendbuf, UART_CMD_CFG, sizeof(sendbuf));

    ret = zb_trigger_send();
    if (ret < 0) {
        LOGW("%s send timeout\r\n",__FUNCTION__);
        return -1;
    }

    return 0;
}

static int zb_cfg_tx_power(int8_t power)
{
    uint8_t sendbuf[2] = {0};
    int ret = -1;

    sendbuf[0] = ZB_CFG_TX_POWER;
    sendbuf[1] = power;

    MKSEND_BUF(sendbuf, UART_CMD_CFG, sizeof(sendbuf));

    ret = zb_trigger_send();
    if (ret < 0) {
        LOGW("%s send timeout\r\n",__FUNCTION__);
        return -1;
    }

    return 0;
}

static void zb_deinit(void)
{
    zb_logic_deinit();
    zb_dev_deinit();
    pthread_mutex_unlock(&devc.init_mutex);
}



static struct uart_zigbee_manager uart_zigbee_manager = {
    .init           = zb_init,
    .reset          = zb_hard_reset,
    .ctrl           = zb_ctrl,
    .get_info       = zb_get_info,
    .factory        = zb_cfg_factory,
    .reboot         = zb_cfg_reboot,
    .set_role       = zb_cfg_role,
    .set_panid      = zb_cfg_pid,
    .set_channel    = zb_cfg_channel,
    .set_key        = zb_cfg_seckey,
    .set_join_aging = zb_cfg_join_aging,
    .set_cast_type  = zb_cfg_cast_type,
    .set_group_id   = zb_cfg_groupid,
    .set_poll_rate  = zb_cfg_poll_rate,
    .set_tx_power   = zb_cfg_tx_power,
    .deinit         = zb_deinit
};

struct uart_zigbee_manager* get_zigbee_manager(void) {
    return &uart_zigbee_manager;
}
