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
#ifndef _APP_POWER_MANAGER_H_
#define _APP_POWER_MANAGER_H_
#include <types.h>


#define DEFAULT_WAKE_LOCK_TIMEOUT_MS        (5000)

struct wake_lock {
    char            name[256];
    int             active;
    int             autosleep_enabled;
};


struct app_power_manager {
    /**
     *    Function: power_off
     * Description: 关机
     *       Input: 无
     *      Return: -1 --> 失败, 成功不需要处理返回值
     */
    int32_t (*power_off)(void);

    /**
     *    Function: reboot
     * Description: 系统复位
     *       Input: 无
     *      Return: -1 --> 失败, 成功不需要处理返回值
     */
    int32_t (*reboot)(void);

    /**
     *    Function: sleep
     * Description: 进入休眠
     *       Input: 无
     *      Return: -1 --> 失败, 0 --> 成功
     */
    int32_t (*sleep)(void);

    int32_t (*wake_lock_init)(struct wake_lock *lock, char *name);
    int32_t (*wake_lock)(struct wake_lock *lock);
    int32_t (*wake_lock_timeout)(struct wake_lock *lock, int timout);
    int32_t (*wake_unlock)(struct wake_lock *lock);
    int32_t (*wake_destroy)(struct wake_lock *lock);
};

/**
 *    Function: get_power_manager
 * Description: 获取 power_manager 句柄
 *       Input: 无
 *      Return: 返回 struct power_manager 结构体指针
 *      Others: 通过该结构体指针访问内部提供的方法
 */
struct app_power_manager *get_app_power_manager(void);


#endif /* POWER_MANAGER_H */
