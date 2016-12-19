/*
 *  Copyright (C) 2016, Wang Qiuwei <qiuwei.wang@ingenic.com, panddio@163.com>
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
#ifndef WATCHDOG_MANAGER_H
#define WATCHDOG_MANAGER_H

struct watchdog_manager {
    /**
     *    Function: init
     * Description: 看门狗初始化
     *       Input:
     *          timeout: 看门狗超时的时间, 以秒为单位, 其值必须大于零
     *      Others: 必须优先调用init函数, 此函数执行成功后, 看门狗就开始倒计时, 可多次调用设置timeout
     *      Return: 0 --> 成功, -1 --> 失败
     */
    int (*init)(unsigned int timeout);

    /**
     *    Function: deinit
     * Description: 看门狗释放
     *       Input: 无
     *      Others: 对应init函数, 不再使用看门狗时调用, 该函数将关闭看门狗, 释放设备
     *      Return: 无
     */
    void (*deinit)(void);

    /**
     *    Function: reset
     * Description: 看门狗喂狗
     *       Input: 无
     *      Others: 在调用init函数后, 在timeout时间内不调用此函数, 系统将复位
     *      Return: 0 --> 成功, -1 --> 失败
     */
    int (*reset)(void);

    /**
     *    Function: enable
     * Description: 看门狗使能
     *       Input: 无
     *      Others: 一般在调用disable函数之后, 需要再重新启动看门狗时, 才调用此函数
     *      Return: 0 --> 成功, -1 --> 失败
     */
    int (*enable)(void);

    /**
     *    Function: disable
     * Description: 看门狗关闭
     *       Input: 无
     *      Others: 此函数区别deinit函数的地方在于, 调用此函数之后, 能通过enable函数重新启动
     *      Return: 0 --> 成功, -1 --> 失败
     */
    int (*disable)(void);
};

/**
 *    Function: get_watchdog_manager
 * Description: 获取 watchdog_manager 句柄
 *       Input: 无
 *      Return: 返回 struct watchdog_manager 结构体指针
 *      Others: 通过该结构体指针访问内部提供的方法
 */
struct watchdog_manager *get_watchdog_manager(void);

#endif /* WATCHDOG_MANAGER_H */
