#ifndef LIBQRCODE_API_H
#define LIBQRCODE_API_H

#include <types.h>
#include <lib/gpio/libgpio.h>

/////////////////////////////////////////////////////COMMON///////////////////////////////////////////////////////////
typedef void (*func_handle)(void *arg);

/////////////////////////////////////////////////////GPIO/////////////////////////////////////////////////////////////
/**
 * GPIO编号
 */
#define GPIO_PA(n)  (0*32 + n)
#define GPIO_PB(n)  (1*32 + n)
#define GPIO_PC(n)  (2*32 + n)
#define GPIO_PD(n)  (3*32 + n)

/**
 * GPIO的中断回调函数
 */
typedef void (*gpio_irq_func)(int);

struct gpio_manager {
    /**
     * Function: manager_init
     * Description: 初始化库资源
     *  注意：必须初始化过一次后才能使用成员功能
     *  Return: 0: 成功， -1: 失败
     */
    int32_t (*init)(void);

    /**
     * Function: manager_deinit
     * Description: 释放库使用的资源
     *  在需不要使用GPIO的情况下，可以释放资源。
     *  注意：在使用GPIO中断时，不能释放库资源
     */
    void (*deinit)(void);

    /**
     * Function: gpio_open
     * Description: GPIO初始化
     * Input:
     *   gpio: 需要操作的GPIO编号
     *        例如: GPIO_PA(n) GPIO_PB(n) GPIO_PC(n) GPIO_PD(n)
     *  注意：操作GPIO前必须先open_gpio
     *  Return: 0: 成功， -1: 失败
     */
    int32_t (*open)(uint32_t gpio);

    /**
     * Function: gpio_close
     * Description: GPIO释放
     * Input:
     *   gpio: 需要操作的GPIO编号
     *        例如: GPIO_PA(n) GPIO_PB(n) GPIO_PC(n) GPIO_PD(n)
     *  不需要用到某个GPIO时，可以调用clsoe来释放资源
     */
    void (*close)(uint32_t gpio);

    /**
     * Function: set_direction
     * Description: 设置GPIO输入输出模式
     * Input:
     *   gpio: 需要操作的GPIO编号
     *        例如: GPIO_PA(n) GPIO_PB(n) GPIO_PC(n) GPIO_PD(n)
     *   dir: 设置功能 输入或输出
     *        参数：GPIO_IN or GPIO_OUT
     *   注意：输出模式下IO默认电平为低
     *  Return: 0: 成功， -1: 失败
     */
    int32_t (*set_direction)(uint32_t gpio, gpio_direction dir);

    /**
     * Function: get_direction
     * Description: 获取GPIO输入输出模式
     * Input:
     *   gpio: 需要操作的GPIO编号
     *        例如: GPIO_PA(n) GPIO_PB(n) GPIO_PC(n) GPIO_PD(n)
     *   dir: 获取功能状态 输入或输出
     *        参数：GPIO_IN or GPIO_OUT
     *  注意：dir参数是gpio_direction指针
     *  Return: 0: 成功， -1: 失败
     */
    int32_t (*get_direction)(uint32_t gpio, gpio_direction *dir);

    /**
     * Function: set_value
     * Description: 设置GPIO电平
     * Input:
     *   gpio: 需要操作的GPIO编号
     *        例如: GPIO_PA(n) GPIO_PB(n) GPIO_PC(n) GPIO_PD(n)
     *   value: 设置电平 低电平或高电平
     *        参数：GPIO_LOW or GPIO_HIGH
     *  注意：输入模式下禁止设置电平
     *  Return: 0: 成功， -1: 失败
     */
    int32_t (*set_value)(uint32_t gpio, gpio_value value);

    /**
     * Function: get_value
     * Description: 获取GPIO电平
     * Input:
     *   gpio: 需要操作的GPIO编号
     *        例如: GPIO_PA(n) GPIO_PB(n) GPIO_PC(n) GPIO_PD(n)
     *   value: 获取电平 低电平或高电平
     *        参数：GPIO_LOW or GPIO_HIGH
     *  注意：value参数是gpio_value指针
     *  Return: 0: 成功， -1: 失败
     */
    int32_t (*get_value)(uint32_t gpio, gpio_value *value);

    /**
     * Function: set_irq_func
     * Description: 使能GPIO中断
     * Input:
     *   func: GPIO中断回调函数
     *       参数：无返回值 和 参数（GPIO的编号）
     *                   typedef void (*irq_work_func)(int);
     *  注意：所有GPIO对于一个中断函数，回调函数参数为触发中断的GPIO编号
     */
    void (*set_irq_func)(gpio_irq_func func);

    /**
     * Function: enable_irq
     * Description: 使能GPIO中断
     * Input:
     *   gpio: 需要操作的GPIO编号
     *        例如: GPIO_PA(n) GPIO_PB(n) GPIO_PC(n) GPIO_PD(n)
     *   mode: 设置中断出发的方式
     *        参数：
     *                       GPIO_RISING,          上升沿触发
     *                       GPIO_FALLING,        下降沿触发
     *                       GPIO_BOTH,             双边沿沿触发
     *  注意：是能前必须设置中断回调函数set_irq_func
     *              GPIO引脚必须为输入模式
     *  Return: 0: 成功， -1: 失败
     */
    int32_t (*enable_irq)(uint32_t gpio, gpio_irq_mode mode);

    /**
     * Function: disable_irq
     * Description: 关闭GPIO中断
     * Input:
     *   gpio: 需要操作的GPIO编号
     *        例如: GPIO_PA(n) GPIO_PB(n) GPIO_PC(n) GPIO_PD(n)
     */
    void (*disable_irq)(uint32_t gpio);
};

/**
 * Function: get_gpio_manager
 * Description: 获取gpio_mananger句柄
 * Input:  无
 * Return: 返回gpio_manager结构体指针
 * Others: 通过该结构体指针访问内部提供的方法
 */
struct gpio_manager* get_gpio_manager(void);

/////////////////////////////////////////////////////I2C//////////////////////////////////////////////////////////////
#define I2C_BUS_MAX            3
/*
 * 读写I2C设备所发送的地址的长度, 以BIT为单位, 有8BIT或16BIT, 应该根据实际使用的器件修改宏值
 */
#define I2C_DEV_ADDR_LENGTH    8
/*
 * 对设备的这个地址进行读操作, 以检测I2C总线上有没有 chip_addr这个从设备, 可根据实际修改宏值
 */
#define I2C_CHECK_READ_ADDR    0x00
/*
 * 对设备一次读写操作后, 在进行下次读写操作时的延时时间,单位:us, 值不能太小, 否则导致读写出错
 */
#define I2C_ACCESS_DELAY_US    5000

/*
 * 定义芯片所支持的所有I2C总线的id, 不能修改任何一个enum i2c成员的值, 否则导致不可预想的错误
 */
enum i2c {
    I2C0,
    I2C1,
    I2C2,
};

/*
 * 应为每个I2C从设备定义一个struct i2c_unit 结构体变量, 每个成员说明如下:
 *        id: 表示从设备所挂载的I2C总戏
 * chip_addr: 为从设备的7位地址
 *
 */
struct i2c_unit {
    enum i2c id;
    int32_t chip_addr;
};

struct i2c_manager {
    /**
     *    Function: init
     * Description: I2C初始化
     *       Input:
     *            i2c: 每个I2C设备对应 struct i2c_unit 结构体指针, 必须先初始化结构体的成员
     *                 其中: id 的值应大于0, 小于I2C_DEVICE_MAX; chip_addr: 为设备的7位地址
     *      Others: 必须优先调用 init函数, 可以被多次调用, 用于初始化不同的I2C设备
     *      Return: 0 --> 成功, 非0 --> 失败
     */
    int32_t (*init)(struct i2c_unit *i2c);

    /**
     *    Function: deinit
     * Description: I2C 设备释放
     *       Input:
     *            i2c: 每个I2C设备对应 struct i2c_unit 结构体指针, 必须先初始化结构体的成员
     *                 其中: id 的值应大于0, 小于I2C_DEVICE_MAX; chip_addr: 为设备的7位地址
     *      Others: 对应于init函数, 不再使用某个I2C设备时, 调用此函数释放, 释放后不能再进行读写操作
     *      Return: 无
     */
    void (*deinit)(struct i2c_unit *i2c);

    /**
     *    Function: read
     * Description: 从I2C设备读取数据
     *       Input:
     *            i2c: 每个I2C设备对应 struct i2c_unit 结构体指针, 必须先初始化结构体的成员
     *                 其中: id 的值应大于0, 小于I2C_DEVICE_MAX; chip_addr: 为设备的7位地址
     *            buf: 存储读取数据的缓存区指针, 不能是空指针
     *           addr: 指定从I2C设备的哪个地址开始读取数据
     *          count: 读取的字节数
     *      Others: 无
     *      Return: 0 --> 成功, -1 --> 失败
     */
    int32_t (*read)(struct i2c_unit *i2c, uint8_t *buf, int32_t addr, int32_t count);

    /**
     *    Function: write
     * Description: 往I2C设备写入数据
     *       Input:
     *            i2c: 每个I2C设备对应 struct i2c_unit 结构体指针, 必须先初始化结构体的成员
     *                 其中: id 的值应大于0, 小于I2C_DEVICE_MAX; chip_addr: 为设备的7位地址
     *            buf: 指向存储待写入数据的缓存区指针, 不能是空指针
     *           addr: 指定从I2C设备的哪个地址开始写入数据
     *          count: 写入的字节数
     *      Others: 无
     *      Return: 0 --> 成功, -1 --> 失败
     */
    int32_t (*write)(struct i2c_unit *i2c, uint8_t *buf, int32_t addr, int32_t count);
};


/**
 *    Function: get_i2c_manager
 * Description: 获取 i2c_manager 句柄
 *       Input: 无
 *      Return: 返回 struct i2c_manager 结构体指针
 *      Others: 通过该结构体指针访问内部提供的方法
 */
struct i2c_manager *get_i2c_manager(void);

/////////////////////////////////////////////////////UART/////////////////////////////////////////////////////////////
#define UART_MAX_CHANNELS   3
/**
 * 奇偶校验可设参数
 */
enum uart_parity {
    /* 无校验 */
    UART_PARITY_NONE = 0,
    /* 奇校验 */
    UART_PARITY_ODD = 1,
    /* 偶校验 */
    UART_PARITY_EVEN = 2,
    /* 校验位总为1 */
    UART_PARITY_MARK = 3,
    /* 校验位总为0 */
    UART_PARITY_SPACE = 4,
};

/**
 * 流控可设参数
 */
enum uart_flowcontrol {
    /* 无流控 */
    UART_FLOWCONTROL_NONE = 0,
    /* 软件流控使用XON/XOFF字符 */
    UART_FLOWCONTROL_XONXOFF = 1,
    /* 硬件流控使用RTS/CTS信号 */
    UART_FLOWCONTROL_RTSCTS = 2,
    /* 硬件流控使用DTR/DSR信号 */
    UART_FLOWCONTROL_DTRDSR = 3,
};

struct uart_manager {
    /**
     * Function: uart_init
     * Description: 串口初始化
     * Input:
     *   devname: 串口设备名称
     *        例如: 普通串口设备/dev/ttySX， usb转串口/dev/ttyUSBX; X为设备序号
     *   baudrate: 波特率 单位:bis per second
     *        波特率取值范围1200~3000000
     *   date_bits: 数据位宽
     *   stop_bits: 停止位宽
     *   parity_bits: 奇偶校验位
     *       可选设置UART_PARITY_NONE, UART_PARITY_ODD, UART_PARITY_EVEN, UART_PARITY_MARK, UART_PARITY_SPACE
     *        分别对应无校验、奇校验、偶校验、校验位总为1、校验位总为0
     *  Output: 无
     *  Return: 0: 成功， -1: 失败
     *  Others: 内部最大可以支持5个uart通道[配置宏UART_MAX_CHANNELS], 每个通道在使用前必须优先调用uart_init
     *              默认流控不开启
     */
    int32_t (*init)(char* devname, uint32_t baudrate, uint8_t date_bits, uint8_t parity,
        uint8_t stop_bits);
    /**
     * Function: uart_deinit
     * Description: 串口释放
     * Input:
     *      devname: 串口设备名称
     *         例如: 普通串口设备/dev/ttySX， usb转串口/dev/ttyUSBX; X为设备序号
     * Output: 无
     * Return: 0: 成功， -1: 失败
     * Others: 对应uart_init, 在不再使用uart时调用
     */
    int32_t (*deinit)(char* devname);
    /**
     * Function: uart_flow_control
     * Description: 串口流控
     * Input:
     *    devname: 串口设备名称
     *         例如: 普通串口设备/dev/ttySX， usb转串口/dev/ttyUSBX; X为设备序号
     *    flow_ctl: 流控选项
     *    UART_FLOWCONTROL_NONE: 无流控
     *    UART_FLOWCONTROL_XONXOFF: 软件流控使用XON/XOFF字符
     *    UART_FLOWCONTROL_RTSCTS: 硬件流控使用RTS/CTS信号
     *    UART_FLOWCONTROL_DTRDSR: 硬件流控使用DTR/DSR信号
     * Output: 无
     * Return: 0: 成功， -1: 失败
     * Others: 无
     */
    int32_t (*flow_control)(char* devname, uint8_t flow_ctl);
    /**
     * Function: uart_write
     * Description: 串口写，在超时时间内写入数据到指定串口
     * Input:
     *     devname: 串口设备名称
     *         例如: 普通串口设备/dev/ttySX， usb转串口/dev/ttyUSBX; X为设备序号
     *    buf: 写数据缓冲区
     *     count: 写数据长度，单位:bytes
     *    timeout_ms: 写超时，单位:ms
     * Output: 无
     * Return: 正数或0: 成功写入的字节数
     * Others: 在使用之前要先调用uart_init
     */
    int32_t (*write)(char* devname, const void* buf, uint32_t count,
        uint32_t timeout_ms);
    /**
     * Function: uart_read
     * Description: 串口读，在超时时间内从指定串口中读出数据
     * Input:
     *    devname: 串口设备名称
     *         例如: 普通串口设备/dev/ttySX， usb转串口/dev/ttyUSBX; X为设备序号
     *    buf: 读数据缓冲区
     *    count: 读数据长度，单位:bytes
     *     timeout_ms: 读超时，单位:ms
     * Output: 无
     * Return: 正数或0: 成功读取的字节数
     * Others: 在使用之前要先调用uart_init
     */
    int32_t (*read)(char* devname, void* buf, uint32_t count, uint32_t timeout_ms);
};

/**
 * Function: get_uart_manager
 * Description: 获取uart_mananger句柄
 * Input:  无
 * Output: 无
 * Return: 返回uart_manager结构体指针
 * Others: 通过该结构体指针访问uart_manager内部提供的方法
 */
struct uart_manager* get_uart_manager(void);

/////////////////////////////////////////////////////TIMER////////////////////////////////////////////////////////////
/* 系统支持的最大定时器个数 */
#define TIMER_DEFAULT_MAX_CNT                            5

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

/////////////////////////////////////////////////////FLASH/////////////////////////////////////////////////////////////
struct flash_manager {
    /**
     * Function: flash_init
     * Description: flash初始化
     * Input: 无
     * Output: 无
     * Return: 0:成功 -1: 失败
     * Others: 在flash的读/写/擦除操作之前，首先执行初始化
     */
    int32_t (*init)(void);
    /**
     * Function: flash_deinit
     * Description: flash释放
     * Input: 无
     * Output: 无
     * Return: 0:成功 -1: 失败
     * Others: 与flash_init相对应
     */
    int32_t (*deinit)(void);
    /**
     * Function: flash_get_erase_unit
     * Description: 获取flash擦除单元， 单位: bytes
     * Input: 无
     * Output: 无
     * Return: >0: 成功返回擦除单元大小  0: 失败
     * Others: 无
     */
    int32_t (*get_erase_unit)(void);
    /**
     * Function: flash_erase
     * Description: flash擦除
     * Input:
     *      offset: flash片内偏移物理地址
     *      length: 擦除大小，单位: byte
     *          注意:该大小必须是擦除单元大小的整数倍
     * Output: 无
     * Return: 0:成功  -1:失败
     * Others: 无
     */
    int64_t (*erase)(int64_t offset,  int64_t length);
    /**
     * Function: flash_read
     * Description: flash读取
     * Input:
     *      offset: flash片内偏移物理地址
     *      buf: 读取缓冲区
     *      length: 读取大小，单位: byte
     * Output: 无
     * Return: >0: 返回成功读取的字节数  -1:失败
     * Others: 无
     */
    int64_t (*read)(int64_t offset,  void* buf, int64_t length);
    /**
     * Function: flash_write
     * Description: flash写入
     * Input:
     *      offset: flash片内偏移物理地址
     *      buf: 写入缓冲区
     *      length: 写入大小，单位: byte
     * Output: 无
     * Return: >0: 返回成功写入的字节数  -1:失败
     * Others: 无
     */
    int64_t (*write)(int64_t offset,  void* buf, int64_t length);
};
/**
 * Function: get_flash_manager
 * Description: 获取flash_mananger句柄
 * Input:  无
 * Output: 无
 * Return: 返回flash_manager结构体指针
 * Others: 通过该结构体指针访问flash_manager内部提供的方法
 */
struct flash_manager* get_flash_manager(void);

//////////////////////////////////////////////CAMERA//////////////////////////////////////////////////////////////////
/*
 * sensor寄存器地址的长度, 以BIT为单位, 有8BIT或16BIT, 应该根据实际使用的sensor修改此宏值
 */
#define SENSOR_ADDR_LENGTH  8

#if (SENSOR_ADDR_LENGTH == 8)
/* 一下三个宏的值不能随便修改, 否则导致不可预知的错误 */
#define ADDR_END    0xff
#define VAL_END     0xff
#define ENDMARKER   {0xff, 0xff}

/*
 * 配置sensor寄存器时, 需传入struct regval_list结构体指针, 以指定配置的寄存器和配置的值
 * regaddr: 寄存器的地址
 *  regval: 对应寄存器的值
 */
struct camera_regval_list {
    uint8_t regaddr;
    uint8_t regval;
};
#elif (SENSOR_ADDR_LENGTH == 16)
/* 一下三个宏的值不能随便修改, 否则导致不可预知的错误 */
#define ADDR_END    0xffff
#define VAL_END     0xff
#define ENDMARKER   {0xffff, 0xff}

/*
 * 配置sensor寄存器时, 需传入struct regval_list结构体指针, 以指定配置的寄存器和配置的值
 * regaddr: 寄存器的地址
 *  regval: 对应寄存器的值
 */
struct camera_regval_list {
    uint16_t regaddr;
    uint8_t regval;
};
#endif

/*
 * 用于设置控制器捕捉图像的分辨率和像素深度, 每个成员说明如下:
 *   width: 图像水平方向的分辨率
 *  height: 图像垂直方向的分辨率
 *     bpp: 图像的像素深度
 *    size: 图像的大小, 字节为单位, size = width * height * bpp / 2
 */
struct camera_img_param {
    uint32_t width;
    uint32_t height;
    uint32_t bpp;    /* bits per pixel: 8/16/32 */
    uint32_t size;
};

/*
 * 用于设置camera 控制器时序的结构, 每个成员说明如下:
 *            mclk_freq: mclk 的频率
 *    pclk_active_level: pclk 的有效电平, 为0是高电平有效, 为1则是低电平有效
 *   hsync_active_level: hsync 的有效电平, 为0是高电平有效, 为1则是低电平有效
 *   vsync_active_level: vsync 的有效电平, 为0是高电平有效, 为1则是低电平有效
 */
struct camera_timing_param {
    uint32_t mclk_freq;
    uint8_t pclk_active_level;
    uint8_t hsync_active_level;
    uint8_t vsync_active_level;
};

struct camera_manager {
    /**
     *    Function: camera_init
     * Description: 摄像头初始化
     *       Input: 无
     *      Others: 必须优先调用 camera_init
     *      Return: 0 --> 成功, 非0 --> 失败
     */
    int32_t (*camera_init)(void);

    /**
     *    Function: camera_deinit
     * Description: 摄像头释放
     *       Input: 无
     *      Others: 对应 camera_init, 不再使用camera时调用
     *      Return: 无
     */
    void (*camera_deinit)(void);

    /**
     *    Function: camera_read
     * Description: 读取摄像头采集数据,保存在 yuvbuf 指向的缓存区中
     *       Input:
     *          yuvbuf: 图像缓存区指针, 缓存区必须大于或等于读取的大小
     *            size: 读取数据大小,字节为单位, 一般设为 image_size
     *      Others: 在此函数中会断言yuvbuf是否等于NULL, 如果为NULL, 将推出程序
     *      Return: 返回实际读取到的字节数, 如果返回-1 --> 失败
     */
    int32_t (*camera_read)(uint8_t *yuvbuf, uint32_t size);

    /**
     *    Function: set_img_param
     * Description: 设置控制器捕捉图像的分辨率和像素深度
     *       Input:
     *           img: struct img_param_t 结构指针, 指定图像的分辨率和像素深度
     *      Return: 0 --> 成功, -1 --> 失败
     */
    int32_t (*set_img_param)(struct camera_img_param *img);

    /**
     *    Function: set_timing_param
     * Description: 设置控制器时序, 包括mclk 频率、pclk有效电平、hsync有效电平、vsync有效电平
     *       Input:
     *          timing: struct timing_param_t 结构指针, 指定 mclk频率、pclk有效电平、hsync有效电平、vsync有效电平
     *                  在camera_init函数中分别初始化为:24000000、0、1、1, 为0是高电平有效, 为1则是低电平有效
     *      Others: 如果在camera_init函数内默认设置已经符合要求,则不需要调用
     *      Return: 0 --> 成功, -1 --> 失败
     */
    int32_t (*set_timing_param)(struct camera_timing_param *timing);

    /**
     *    Function: sensor_setup_addr
     * Description: 设置摄像头sensor的I2C地址 (在 probe 失败时,应该调用此函数设置sensor的I2C地址)
     *       Input:
     *              chip_addr: 摄像头sensor的I2C地址(不需要右移一位)
     *      Return: 0 --> 成功, -1 --> 失败
     */
    int32_t (*sensor_setup_addr)(int32_t chip_addr);

    /**
     *    Function: sensor_setup_regs
     * Description: 设置摄像头sensor的多个寄存器,用于初始化sensor
     *       Input:
     *          vals: struct regval_list 结构指针, 通常传入struct regval_list结构数组
     *      Others: 在开始读取图像数据时, 应该调用此函数初始化sensor寄存器
     *      Return: 0 --> 成功, -1 --> 失败
     */
    int32_t (*sensor_setup_regs)(const struct camera_regval_list *vals);

    /**
     *    Function: sensor_write_reg
     * Description: 设置摄像头sensor的单个寄存器
     *       Input:
     *          regaddr: 摄像头sensor的寄存器地址
     *           regval: 摄像头sensor寄存器的值
     *      Return: 0 --> 成功, -1 --> 失败
     */
    int32_t (*sensor_write_reg)(uint32_t regaddr, uint8_t regval);

    /**
     *    Function: sensor_read_reg
     * Description: 读取摄像头sensor某个寄存器的知
     *       Input:
     *          regaddr: 摄像头sensor的寄存器地址
     *      Return: -1 --> 失败, 其他 --> 寄存器的值
     */
    uint8_t (*sensor_read_reg)(uint32_t regaddr);
};

/**
 *    Function: get_camera_manager
 * Description: 获取 camera_mananger 句柄
 *       Input: 无
 *      Return: 返回 camera_manager 结构体指针
 *      Others: 通过该结构体指针访问内部提供的方法
 */
struct camera_manager *get_camera_manager(void);

/////////////////////////////////////////////////////PWM//////////////////////////////////////////////////////////////
#define PWM_CHANNEL_MAX   5

/* PWM 频率的最小值 */
#define PWM_FREQ_MIN     200
/* PWM 频率的最大值 */
#define PWM_FREQ_MAX     100000000

/*
 * 定义芯片所支持的所有PWM通道的id, 不能修改任何一个enum pwm成员的值
 * 不能修改任何一个enum pwm成员的值, 否则导致不可预知的错误
 */
enum pwm {
    PWM0,
    PWM1,
    PWM2,
    PWM3,
    PWM4,
};

/*
 * 定义PWM通道的工作状态, 用于 setup_state 函数设置PWM通道的工作状态
 * 不能修改任何一个enum pwm_state成员的值, 否则会PWM通道工作状态设置错误
 */
enum active {
    ACTIVE_LOW,
    ACTIVE_HIGH,
};

/*
 * 定义PWM通道的工作状态, 用于 setup_state 函数设置PWM通道的工作状态
 * 不能修改任何一个enum pwm_state成员的值, 否则会PWM通道工作状态设置错误
 */
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
    int32_t (*init)(enum pwm id, enum active level);

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
    int32_t (*setup_freq)(enum pwm id, uint32_t freq);

    /**
     *    Function: setup_duty
     * Description: 设置PWM通道的占空比
     *       Input:
     *              id: PWM通道id, 其值必须小于PWM_DEVICE_MAX
     *            duty: 占空比, 其值为 0 ~ 100 区间
     *      Others: 初始化占空比为 0,这里不用关心IO输出的有效电平,例如:不管LED在低电平亮还是高电平亮, duty=100时, LED最亮
     *      Return: 0 --> 成功, -1 --> 失败
     */
    int32_t (*setup_duty)(enum pwm id, uint32_t duty);

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
    int32_t (*setup_state)(enum pwm id, enum pwm_state state);
};

/**
 *    Function: get_pwm_manager
 * Description: 获取 pwm_manager 句柄
 *       Input: 无
 *      Return: 返回 struct pwm_manager 结构体指针
 *      Others: 通过该结构体指针访问内部提供的方法
 */
struct pwm_manager *get_pwm_manager(void);

/////////////////////////////////////////////////////WATCHDOG/////////////////////////////////////////////////////////
struct watchdog_manager {
    /**
     *    Function: init
     * Description: 看门狗初始化
     *       Input:
     *          timeout: 看门狗超时的时间, 以秒为单位, 其值必须大于零
     *      Others: 必须优先调用init函数初始化看门狗和设置timeout, 可被多次调用
     *      Return: 0 --> 成功, -1 --> 失败
     */
    int32_t (*init)(uint32_t timeout);

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
     *      Others: 使能看门狗后, 在timeout时间内不调用此函数, 系统将复位
     *      Return: 0 --> 成功, -1 --> 失败
     */
    int32_t (*reset)(void);

    /**
     *    Function: enable
     * Description: 看门狗使能
     *       Input: 无
     *      Others: 在init函数初始化或disable函数关闭看门狗之后, 调用此函数启动看门狗
     *      Return: 0 --> 成功, -1 --> 失败
     */
    int32_t (*enable)(void);

    /**
     *    Function: disable
     * Description: 看门狗关闭
     *       Input: 无
     *      Others: 对应enable函数, 区别deinit函数的地方在于, 调用此函数之后, 能通过enable函数重新启动
     *      Return: 0 --> 成功, -1 --> 失败
     */
    int32_t (*disable)(void);
};

/**
 *    Function: get_watchdog_manager
 * Description: 获取 watchdog_manager 句柄
 *       Input: 无
 *      Return: 返回 struct watchdog_manager 结构体指针
 *      Others: 通过该结构体指针访问内部提供的方法
 */
struct watchdog_manager *get_watchdog_manager(void);

/////////////////////////////////////////////////////POWER////////////////////////////////////////////////////////////
struct power_manager {
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
};

/**
 *    Function: get_power_manager
 * Description: 获取 power_manager 句柄
 *       Input: 无
 *      Return: 返回 struct power_manager 结构体指针
 *      Others: 通过该结构体指针访问内部提供的方法
 */
struct power_manager *get_power_manager(void);
#endif
