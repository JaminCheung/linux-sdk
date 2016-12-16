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
#ifndef PWM_MANAGER_H
#define PWM_MANAGER_H

#define PWM_DEVICE_MAX   5
#define PWM_PERIOD_MIN   200
#define PWM_PERIOD_MAX   100000000

enum pwm {
    PWM0,
    PWM1,
    PWM2,
    PWM3,
    PWM4,
};

enum pwm_state {
    PWM_DISABLE,
    PWM_ENABLE,
};

struct pwm_manager {
    /**
     *    Function: init
     * Description: PWM通道初始化
     *       Input:
     *              id: PWM通道id, 其值必须小于PWM_DEVICE_MAX
     *      Others: 必须优先调用 pwm_init函数
     *      Return: 0 --> 成功, 非0 --> 失败
     */
    int (*init)(enum pwm id);

    /**
     *    Function: deinit
     * Description: PWM通道释放
     *       Input:
     *              id: PWM通道id, 其值必须小于PWM_DEVICE_MAX
     *      Others: 对应于init函数, 不再使用PWM某个通道时,应该调用此函数释放
     *      Return: 无
     */
    void (*deinit)(enum pwm id);

    /**
     *    Function: setup_freq
     * Description: 设置PWM通道的频率
     *       Input:
     *              id: PWM通道id, 其值必须小于PWM_DEVICE_MAX
     *            freq: 周期值, ns 为单位
     *      Others: 此函数可以不调用, 即使用默认频率: 30000ns
     *      Return: 0 --> 成功, -1 --> 失败
     */
    int (*setup_freq)(enum pwm id, unsigned int freq);

    /**
     *    Function: setup_duty
     * Description: 设置PWM通道的占空比
     *       Input:
     *              id: PWM通道id, 其值必须小于PWM_DEVICE_MAX
     *            duty: 占空比, 其值为 0 ~ 100 区间
     *      Others: 初始化占空比为 0,这里不用关心IO输出的有效电平,例如:不管LED在低电平亮还是高电平亮, duty=100时, LED最亮
     *      Return: 0 --> 成功, -1 --> 失败
     */
    int (*setup_duty)(enum pwm id, unsigned int duty);

    /**
     *    Function: setup_state
     * Description: 设置PWM通道的工作状态
     *       Input:
     *             id: PWM通道id, 其值必须小于PWM_DEVICE_MAX
     *          state: 工作状态: 0 --> disable, 非0 --> enable
     *      Others: 此函数不需要在 setup_freq 或 setup_duty 之前调用,主要用于暂停/开始PWM的工作.
     *              再重新开始工作时,PWM保持之前的 freq 和 duty 继续工作
     *      Return: 0 --> 成功, -1 --> 失败
     */
    int (*setup_state)(enum pwm id, enum pwm_state state);
};

/**
 *    Function: get_pwm_manager
 * Description: 获取 pwm_manager 句柄
 *       Input: 无
 *      Return: 返回 struct pwm_manager 结构体指针
 *      Others: 通过该结构体指针访问内部提供的方法
 */
struct pwm_manager *get_pwm_manager(void);
#endif /* PWM_MANAGER_H */
