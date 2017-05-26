/*
 *  Copyright (C) 2016, Zhang YanMing <yanmin.zhang@ingenic.com, jamincheung@126.com>
 *
 *  Ingenic QRcode SDK Project
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

#ifndef TIMER_MANAGER_H
#define TIMER_MANAGER_H
#include <types.h>
typedef void (*func_handle)(void *arg);

/* 系统支持的最大定时器个数 */
#define TIMER_DEFAULT_MAX_CNT                            3

struct timer_manager {
    /**
     * Function: timer_init
     * Description: 定时器初始化
     * Input:
     *  id: 指定分配的id号,可选配置有以下两类
     *      id=-1: 自动分配定时器id
     *      id>=1: 固定分配特定的id给定时器,有效范围为[1,TIMER_DEFAULT_MAX_CNT]
     *  interval: 定时周期,单位:ms
     *  is_one_time: 是否是一次定时, 大于0为一次定时,否则周期定时
     *  routine: 定时器处理函数
     *  arg: 定时器处理函数参数
     *       注意: arg为指针,sdk中只是传递指针, 指针指向的内容请用户注意保护
     * Output: 无
     * Return: >=1:返回成功分配的id号 -1: 失败
     * Others: 支持的最大定时器数目由宏定义TIMER_DEFAULT_MAX_CNT决定
     */
    int32_t (*init)(int32_t id, uint32_t interval, uint8_t is_one_time,
            func_handle routine, void *arg);
    /**
     * Function: timer_deinit
     * Description: 定时器释放
     * Input:
     *      id: 定时器id号, id号有效范围为[1,TIMER_DEFAULT_MAX_CNT]
     * Output: 无
     * Return: 0:成功 -1: 失败
     * Others: 与timer_init相对应
     */
    int32_t (*deinit)(uint32_t id);
    /**
     * Function: timer_start
     * Description: 定时器开启, 调用成功后定时器执行定时计数
     * Input:
     *  id: 定时器id号, id号有效范围为[1,TIMER_DEFAULT_MAX_CNT]
     * Output: 无
     * Return: 0:成功 -1: 失败
     * Others: 无
     */
    int32_t (*start)(uint32_t id);
     /**
     * Function: timer_stop
     * Description: 定时器停止, 调用成功后定时器停止定时计数
     * Input:
     *  id: 定时器id号, id号有效范围为[1,TIMER_DEFAULT_MAX_CNT]
     * Output: 无
     * Return: 0:成功 -1: 失败
     * Others: 与timer_start相对应
     *         调用stop后定时器被终止,下次调用start时,定时器按照init的参数重新定时计数
     */
    int32_t (*stop)(uint32_t id);
    /**
     * Function: timer_get_counter
     * Description: 返回本次定时剩余时间,单位:ms
     * Input:
     *  id: 定时器id号, id号有效范围为[1,TIMER_DEFAULT_MAX_CNT]
     * Output: 无
     * Return: >=0:返回本次定时剩余时间 -1: 失败
     * Others: 无
     */
    int64_t (*get_counter)(uint32_t id);
};

/**
 * Function: get_timer_manager
 * Description: 获取get_timer_manager句柄
 * Input:  无
 * Output: 无
 * Return: 返回uart_manager结构体指针
 * Others: 通过该结构体指针访问timer_manager内部提供的方法
 */
struct timer_manager*  get_timer_manager(void);

#endif
