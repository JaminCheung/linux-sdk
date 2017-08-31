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
#include <zigbee/protocol/uart_protocol.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <utils/assert.h>
#include <utils/log.h>

#define LOG_TAG                         "uart_protocol"

enum recv_status_e{
    UART_RECV_STATUS_SYNC1 = 0,
    UART_RECV_STATUS_SYNC2,
    UART_RECV_STATUS_LEN_H,
    UART_RECV_STATUS_LEN_L,
    UART_RECV_STATUS_PL
};


#define UART_SYNC_FE                    0xFE
#define UART_TRANS_5A                   0x5A

//position of segment
#define UART_SGM_SYNC                   0x00
#define UART_SGM_LEN                    0x02
#define UART_SGM_CMD                    0x04
#define UART_SGM_SN                     0x05
#define UART_SGM_PL                     0x06

#define UART_HEAD_LEN                   (2+2+1+1)        //sync 2B + len 2B + cmd 1B + sn 1B

//#define UART_RECV_BUF_LEN     128


#define rbuf_rw_mask(x)                 ((x)&(src->rbuf_len-1))
#define rbuf_read(s)                    src->rbuf[rbuf_rw_mask(s)]
#define rbuf_write(s,v)                 src->rbuf[rbuf_rw_mask(s)] = (v)
#define rbuf_cut_byte(x)                src->read += (x)
#define rbuf_is_empty()                 ((rbuf_rw_mask(src->write))==(rbuf_rw_mask(src->read)))
#define rbuf_clean()                    src->read = src->write = 0
#define rbuf_read_lseek(x)              src->read = (x)
#define rbuf_write_lseek(x)             src->write = (x)


static inline uint16_t rbuf_free_size(pro_src_st* src);
static inline uint16_t rbuf_data_size(pro_src_st* src);
static inline uint16_t pack_build(uint8_t cmd, uint8_t* pl, uint16_t pllen, uint8_t* buf);


static inline uint16_t rbuf_free_size(pro_src_st* src)
{
    if (rbuf_rw_mask(src->write) >= rbuf_rw_mask(src->read))
        return src->rbuf_len - rbuf_rw_mask(src->write) + rbuf_rw_mask(src->read) - 1;
    else
        return rbuf_rw_mask(src->read) - rbuf_rw_mask(src->write) - 1;
}

static inline uint16_t rbuf_data_size(pro_src_st* src)
{
    if (rbuf_rw_mask(src->write) >= rbuf_rw_mask(src->read))
        return rbuf_rw_mask(src->write) - rbuf_rw_mask(src->read);
    else
       return src->rbuf_len - rbuf_rw_mask(src->read) + rbuf_rw_mask(src->write);
}

static uint16_t gen_checksum(uint8_t *buf, uint16_t len)
{
    uint16_t i,checksum = 0;

    for (i=0; i<len; i++)
        checksum += buf[i];
    return checksum;
}

static uint8_t gen_sn(void)
{
    static uint8_t sn = 1;

    if (sn == 0)
        sn = 1;

    return sn++;
}

__attribute__((unused)) static void forearch_buf(pro_src_st* src)
{
    int16_t i,read = src->read;

    LOGD("read 0x%x, write 0x%x, bufdata: %d, freesize: %d\r\n",src->read,src->write,
                                              rbuf_data_size(src),rbuf_free_size(src));
    for (i=0; i<rbuf_data_size(src); i++) {
        printf("0x%x ", src->rbuf[rbuf_rw_mask(read++)]);
    }
    printf("\r\n");

    return;
}

/**
 *  @fn      buf_mov_r_a_byte
 *
 *  @brief   straight buffer data move a byte to the right,
 *           user translation for send buffer  to insert 0x5A
 *
 *  @param   buf - the buf of starting position of the move
 *  @param   len - the length of the data being moved
 *
 *  @return  none
 */
static void buf_mov_r_a_byte(uint8_t* buf, uint16_t len)
{
    int16_t i;

    for (i=len; i>0; i--)
        buf[i] = buf[i-1];

    return;
}


/**
 *  @fn      rbuf_mov_l_a_byte
 *
 *  @brief   ring buffer data move a byte to the left
 *
 *  @param   src - application module buffer resources
 *  @param   ls  - the buf of starting position of the move
 *
 *  @return  none
 */

__attribute__((unused)) static void rbuf_mov_l_a_byte(pro_src_st* src, uint16_t ls)
{
    uint16_t i,ml;

    if (rbuf_rw_mask(ls) < rbuf_rw_mask(src->write))
        ml = rbuf_rw_mask(src->write) - rbuf_rw_mask(ls);
    else
        ml = src->rbuf_len-ls+rbuf_rw_mask(src->write);

    for (i=0; i < ml; i++) {
        //move the buffer data one byte to the left
        rbuf_write(ls+i, rbuf_read(ls+1+i));
    }
    src->write--;
}


/**
 *  @fn      extract_trans_char
 *
 *  @brief   to extract the segment of 0xFE when it was received.
 *
 *  @param   src - application module buffer resources
 *  @param   status - pointer, the data parsed state of the state machine
 *  @param   ls  - pointer, the current position of reading
 *
 *  @return  0 - success
 *           -1 - illegal packet
 */
static int8_t extract_trans_char(pro_src_st* src,
                        enum recv_status_e* status, uint16_t* ls)
{
    if (rbuf_read(*ls+1) == UART_SYNC_FE) {
        /* sync head first 0xFE */
        *status = UART_RECV_STATUS_SYNC1;
        rbuf_read_lseek(*ls);
        return 0;
    }else if (rbuf_read(*ls-1) == UART_SYNC_FE) {
        /* sync head second 0xFE */
        *status = UART_RECV_STATUS_SYNC2;
        rbuf_read_lseek(*ls-1);
        return 0;
    /* skip translation char 0x5A */
    }else if (rbuf_read(*ls+1) == UART_TRANS_5A &&
              *status > UART_RECV_STATUS_LEN_H) {
        (*ls)++;
        return 0;
    }else{
        *status = UART_RECV_STATUS_SYNC1;
        (*ls)++;
        rbuf_read_lseek(*ls);
        /* illegal packet */
        return -1;
    }

    return 0;
}

/**
 *  @fn      extract_pro_pkg
 *
 *  @brief   extract a complete protocol packet
 *
 *  @param   src - application module buffer resources
 *  @param   outbuf : extract a complete protocol packet
 *
 *  @return  >0 - complete protocol packet length
 *           -1 - illegal packet
 */

static int16_t extract_pro_pkg(pro_src_st* src,uint8_t* outbuf)
{
    uint8_t data;
    enum recv_status_e recv_status = UART_RECV_STATUS_SYNC1;
    uint16_t i,slen = 0,recv_len = 0;
    uint16_t p_read = src->read;

    LOGD("read 0x%x, write 0x%x, tmp_read 0x%x\r\n",src->read,src->write,p_read);
    while((rbuf_rw_mask(src->write)!=(rbuf_rw_mask(p_read)))) {
        data = rbuf_read(p_read);
        //LOGD("get ------------------------- %x, status %d\r\n", data,recv_status);
        /* receive 0xFE, extract it is payload or sync head? */
        if (data == UART_SYNC_FE) {
            if (extract_trans_char(src, &recv_status,&p_read) < 0) {
                continue;
            }
        }

        switch(recv_status) {
            case UART_RECV_STATUS_SYNC1:
                if (UART_SYNC_FE == data) {
                    recv_status = UART_RECV_STATUS_SYNC2;
                    recv_len = 1;
                }else{
                    /* expect to receive 0xFE, but not, cut a byte. */
                    rbuf_cut_byte(1);
                }
            break;

            case UART_RECV_STATUS_SYNC2:
                if (UART_SYNC_FE == data) {
                    recv_status = UART_RECV_STATUS_LEN_H;
                    recv_len++;
                }else{
                    /* expect to receive second 0xFE, but not, cut 2 byte.*/
                    recv_status = UART_RECV_STATUS_SYNC1;
                    rbuf_cut_byte(2);
                    recv_len = 0;
                }
            break;

            case UART_RECV_STATUS_LEN_H:
                if (UART_SYNC_FE == data) {
                    recv_status = UART_RECV_STATUS_LEN_H;
                    rbuf_cut_byte(1);
                    recv_len = 2;
                }else{
                    slen = data;
                    recv_status = UART_RECV_STATUS_LEN_L;
                    recv_len++;
                }
            break;

            case UART_RECV_STATUS_LEN_L:
                slen  = ((slen<<8)&0xFF00)|data;
                recv_status = UART_RECV_STATUS_PL;
                recv_len++;
            break;

            case UART_RECV_STATUS_PL:
                recv_len++;
                //LOGD("recv_len: %d,slen: %d\r\n",recv_len,slen);
                if (recv_len == slen+2+2) {
                    i = 0;
                    while(recv_len--) {
                        if (!rbuf_is_empty()) {
                            /* moving actual read pointer */
                            outbuf[i++] = rbuf_read(src->read++);
                            if (i>2 && outbuf[i-1] == UART_SYNC_FE)
                                rbuf_cut_byte(1);
                        }else{
                            return -1;
                        }
                    }
                    /* a whole packet be read and return length */
                    return i;
                }
            break;

            default:
            break;
        }
        /* moving a temporary pointer is not an actual read pointer */
        p_read++;
    }

    return -1;
}


/**
 *  @fn      pack_build
 *
 *  @brief   build packet format
 *
 *  @param   cmd - cmd
 *  @param   pl - payload
 *  @param   pllen - payloda length
 *  @param   buf - built packet
 *
 *  @return  length of built packet
 */
static inline uint16_t pack_build(uint8_t cmd, uint8_t* pl,
                                  uint16_t pllen, uint8_t* buf)
{
    uint16_t checksum;

    buf[UART_SGM_SYNC]         = 0xFE;
    buf[UART_SGM_SYNC+1]       = 0xFE;
    //len = pllen + cmd  1B + sn 1B  + checksum 2B
    buf[UART_SGM_LEN]          = (uint8_t)(((pllen+4)&0xFF00)>>8);
    buf[UART_SGM_LEN+1]        = (uint8_t)((pllen+4)&0x00FF);
    buf[UART_SGM_CMD]          = cmd;
    buf[UART_SGM_SN]           = gen_sn();
    if (pllen > 0 && pl != NULL) {
        memcpy(&buf[UART_SGM_PL], pl, pllen);
    }
    checksum                   = gen_checksum(buf, pllen+UART_HEAD_LEN);
    buf[pllen+UART_HEAD_LEN]   = (uint8_t)((checksum&0xFF00)>>8);
    buf[pllen+UART_HEAD_LEN+1] = (uint8_t)(checksum&0x00FF);

    //return all len ( + check sum)
    return (pllen+UART_HEAD_LEN+2);
}


/**
 *  @fn      pack_translate
 *
 *  @brief   insert 0x5A for behind of 0xFE in built packet
 *
 *  @param   buf - packet be built
 *  @param   len - payloda length
 *
 *  @return  length of translated packet
 */
static int16_t pack_translate(uint8_t* buf, uint16_t len)
{
    uint16_t i,slen = len;

    for (i=2; i<len; i++) {
        if (buf[i] == UART_SYNC_FE) {
            buf_mov_r_a_byte(&buf[i+1], len-i-1);
            buf[++i] = UART_TRANS_5A;
            len++;
            slen++;
        }
    }

    return slen;
}

/**
 *  @fn      pack_handle
 *
 *  @brief   handle a complete protocol packet after extracted
 *
 *  @param   buf - complete protocol packet
 *  @param   len - length of packet
 *  @param   cb - application income callback function
 *
 *  @return  0 - success
 *           -1 - illegal entry
 *           -2 - checksum error
 */
static int16_t pack_handle(uint8_t* buf, uint16_t len, uart_pro_recv_cb cb)
{
    uint8_t cmd        = buf[UART_SGM_CMD];
    uint8_t sn         = buf[UART_SGM_SN];
    uint16_t cchecksum = gen_checksum(buf,len-2);
    uint16_t rchecksum = buf[len-2]<<8 | buf[len-1];

    assert_die_if(cb == NULL, "cb is NULL\n");
    assert_die_if(buf == NULL, "buf is NULL\n");

#ifdef LOCAL_DEBUG
    LOGD("-------------------------------\n");
    LOGD("extract %d:", len);
    for (int i=0; i<len; i++)
        printf(" 0x%x",buf[i]);
    printf("\r\n");
    LOGD("-------------------------------\n");
#endif

    if (cchecksum != rchecksum) {
        LOGW("calc checksum :0x%x, recv checksum: 0x%x\r\n",cchecksum,rchecksum);
        return -1;
    }
    LOGD("cmd :0x%x, sn: 0x%x\r\n",cmd,sn);
    cb(cmd,&buf[UART_SGM_PL],len-8);          // sync 2B+len 2B+cmd1B+sn1B+chksum2B

    return 0;
}


/**
 *  @fn      uart_pro_init
 *
 *  @brief   malloc memory space for buffer in src
 *
 *  @param   src - application module buffer resources
 *
 *  @return  0 - success
 *           -1 - failed
 */

static int16_t uart_pro_init(pro_src_st* src)
{
    assert_die_if(src == NULL, "src is NULL\n");

    src->rbuf = (uint8_t*)malloc(src->rbuf_len);
    if (src->rbuf == NULL) {
        return -1;
    }
    src->pbuf = (uint8_t*)malloc(src->pbuf_len);
    if (src->pbuf == NULL) {
        free(src->rbuf);
        return -1;
    }
    /* len*2:  need insert 0x5A to translate 0xFE */
    src->sbuf = (uint8_t*)malloc((src->sbuf_len)*2);
    if (src->pbuf == NULL) {
        free(src->pbuf);
        free(src->rbuf);
        return -1;
    }

    return 0;
}


/**
 *  @fn      uart_pro_handle
 *
 *  @brief   handle uart data of received, should be call after receive function
 *           it is best to call in the message processing instead of uart irq service function
 *
 *  @param   src - application module buffer resources
 *  @param   cb - application income callback function
 *
 */
static void uart_pro_handle(pro_src_st* src, uart_pro_recv_cb cb)
{
    int16_t rl;

    assert_die_if(cb == NULL, "cb is NULL\n");
    assert_die_if(src == NULL, "src is NULL\n");

    do {
        rl = extract_pro_pkg(src, src->pbuf);
        if (rl > 0) {
            pack_handle(src->pbuf,rl,cb);
        }
  } while(rl > 0);
}


/**
 *  @fn      uart_pro_receive
 *
 *  @brief   receive uart data to the ring buffer in src, in the uart service function call
 *
 *  @param   src - application module buffer resources
 *  @param   buf - uart data
 *  @param   len - uart data length
 *
 *  @return  received length
 */
static int16_t uart_pro_receive(pro_src_st* src, uint8_t* buf, uint16_t len)
{
    uint16_t i = 0;

    assert_die_if(buf == NULL, "buf is NULL\n");
    assert_die_if(src == NULL, "src is NULL\n");
    assert_die_if(len == 0, "len is error\n");

#ifdef LOCAL_DEBUG
    LOGD("-------------------------------\n");
    LOGD("uart recv %d: ", len);
    for (i=0; i<len; i++) {
        printf(" 0x%x",buf[i]);
        rbuf_write(src->write++,buf[i]);
        if (rbuf_free_size(src) <= 0) {
        break;
      }
    }
    printf("\r\n");
    LOGD("-------------------------------\n");
#else
    for (i=0; i<len; i++) {
        rbuf_write(src->write++,buf[i]);
        if (rbuf_free_size(src) <= 0) {
        break;
      }
    }
#endif
    return i;
}


/**
 *  @fn      uart_pro_send
 *
 *  @brief   encapsulates the data into a protocol packet to send
 *
 *  @param   src - application module buffer resources
 *  @param   cmd - cmd
 *  @param   pl  - payload
 *  @param   pllen - payload data length
 *  @param   hd  - data send handle function, income application module
 *
 *  @return  hd return value
 *
 */
static int16_t uart_pro_send(pro_src_st* src, uint8_t cmd,
                        uint8_t* pl, uint16_t pllen,uart_pro_send_hd hd)
{
    int ret;
    uint16_t len,slen;

    assert_die_if(hd == NULL, "hd is NULL\n");
    assert_die_if(src == NULL, "src is NULL\n");

    len = pack_build(cmd, pl, pllen, src->sbuf);
    slen = pack_translate(src->sbuf, len);

#ifdef LOCAL_DEBUG
    LOGD("-------------------------------\n");
    LOGD("send %d:", slen);
    for (int i=0; i<slen; i++)
        printf(" 0x%x",src->sbuf[i]);
    printf("\r\n");
    LOGD("-------------------------------\n");
#endif

    ret = hd(src->sbuf,slen);

    return ret;
}

/**
 *  @fn      uart_pro_deinit
 *
 *  @brief   free memory space for buffer in src
 *
 *  @param   src - application module buffer resources
 *
 */
static void uart_pro_deinit(pro_src_st* src)
{
    assert_die_if(src == NULL, "src is NULL\n");

    if (src->rbuf != NULL) {
       free(src->rbuf);
    }
    if (src->rbuf != NULL) {
       free(src->pbuf);
    }
    if (src->rbuf != NULL) {
       free(src->sbuf);
    }
}

static struct uart_pro_manager uart_pro_manager = {
    .init     = uart_pro_init,
    .receive  = uart_pro_receive,
    .send     = uart_pro_send,
    .handle   = uart_pro_handle,
    .deinit   = uart_pro_deinit,
};

struct uart_pro_manager* get_uart_pro_manager(void)
{
    return &uart_pro_manager;
}
