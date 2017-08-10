#ifndef _CAMERA_V4L2_MANAGER_H_
#define _CAMERA_V4L2_MANAGER_H_


typedef enum {
    METHOD_READ,
    METHOD_MMAP,
    METHOD_USERPTR,
} io_m;



typedef void (*frame_receive)(char* buf, unsigned int width, unsigned int height, unsigned int seq);

struct capt_param_t {
    unsigned int width;         // Resolution width(x)
    unsigned int height;        // Resolution height(y)
    unsigned int bpp;           // Bits Per Pixels
    unsigned int nbuf;          // buf queue
    frame_receive fr_cb;
    io_m         io;            // method of get image data
};




struct camera_v4l2_manager {
    /**
     *  @brief   初始化camera devide
     *
     *  @param   capt_p - 传入获取图像的参数
     *
     */
    void (*init)(struct capt_param_t *capt_p);
    /**
     *  @brief   开启图像获取 ，目前只支持LCD预览，内部开启线程循环
     *
     */
    void (*start)(void);
    /**
     *  @brief   关闭图像获取 ，关闭线程
     *
     */
    void (*stop)(void);
    /**
     *  @brief   释放模块
     *
     */
    int (*yuv2rgb)(unsigned char* yuv, unsigned char* rgb, unsigned int width, unsigned int height);

    int (*rgb2pixel)(unsigned char* rgb, unsigned short* pbuf, unsigned int width, unsigned int height);

    int (*build_bmp)(unsigned char* rgb, unsigned int width, unsigned int height, unsigned char* filename);

    void (*deinit)(void);
};

struct camera_v4l2_manager* get_camera_v4l2_manager(void);

#endif
