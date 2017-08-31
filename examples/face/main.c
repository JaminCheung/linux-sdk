/*
 *  Copyright (C) 2017, Monk Su<rongjin.su@ingenic.com, MonkSu@outlook.com>
 *
 *  Ingenic SDK Project
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
#include <assert.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <utils/log.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <linux/rtc.h>


#include <fb/fb_manager.h>
#include <utils/png_decode.h>
#include <graphics/gr_drawer.h>
#include <input/input_manager.h>
#include <lib/face/NmIrFaceSdk.h>
#include <camera_v4l2/camera_v4l2_manager.h>


typedef enum {
    CHANNEL_SEQUEUE_COLOR       = 0x00,
    CHANNEL_SEQUEUE_BLACK_WHITE = 0x01
} chselect_m;


typedef enum {
    RECOGNIZE_IDLE              = 0x00,
    RECOGNIZE_RECORD            = 0x01,
    RECOGNIZE_COMPARE           = 0X02
} rec_tep_m;

typedef enum {
    RECOGNIZE_PIXELS_240X320    = 0x00,
    RECOGNIZE_PIXELS_320X240    = 0x01
} rec_pix_m;

/*
 * Macro
 */


#define LOG_TAG                         "face_recognize"


#define RECOGNIZE_PIXELS

#define LIST_MAX_ENTRY                   20

#define DEFAULT_WIDTH                    320
#define DEFAULT_HEIGHT                   240
#define DEFAULT_BPP                      16
#define DEFAULT_NBUF                     4


#define SIMILARITY_THRESHOLD             70


#define PNG_RECORD_SUCCESSFUL            0x00
#define PNG_MATCH_SUCCESSFUL             0x01
#define PNG_MATCH_FAULT                  0x02




#define SET_RECORD_NUM(x)               face_info.record_num = x

#define SET_RECOGNIZE_STEP(x)           face_info.recognize_tep = x
#define GET_RECOGNIZE_STEP()            face_info.recognize_tep

#define MK_PIXEL_FMT(x)                                 \
        do{                                             \
            x.rbit_len = fbm->get_redbit_length();      \
            x.rbit_off = fbm->get_redbit_offset();      \
            x.gbit_len = fbm->get_greenbit_length();    \
            x.gbit_off = fbm->get_greenbit_offset();    \
            x.bbit_len = fbm->get_bluebit_length();     \
            x.bbit_off = fbm->get_bluebit_offset();     \
        }while(0)

/*
 *  VARIABLE
 */

static struct camera_v4l2_manager* cimm;
static struct fb_manager* fbm;
static struct gr_drawer* gr_drawer;
static struct input_manager *inputm;

char* step_str[] = {
    "Idle",
    "Record",
    "Compare"
};

static struct _capt_op {
    chselect_m view_channel;     // select view channel
    chselect_m recognize_channel;// select recognize channel
    uint16_t *ppbuf;       // map lcd piexl buf
    int lock_time_ms;
}capt_op;


static struct _face_info {
    int record_num;
    RECT rect;
    POINT pt_right_eye;
    POINT pt_left_eye;
    uint8_t* p_feature;
    HANDLE p_list_handle;
    rec_tep_m recognize_tep;
    rec_pix_m recognize_pix;
} face_info = {
    .record_num    = 0,
    .recognize_tep = RECOGNIZE_IDLE,
    .recognize_pix = RECOGNIZE_PIXELS_240X320
};

static struct _drawer_op {
    struct gr_surface* p_surface;
    uint32_t pos_x;
    uint32_t pos_y;
}draw_pngs[3];



static void input_event_listener(const char* input_name, struct input_event* event)
{
    if (event->code == KEY_POWER) {
        SET_RECOGNIZE_STEP(RECOGNIZE_RECORD);
        LOGI("now step in record\n");
    } else if (event->code == KEY_F1) {
        SET_RECOGNIZE_STEP(RECOGNIZE_COMPARE);
        LOGI("now step in compare\n");
    }
}

static long get_dtime()
{
    struct  timeval tv;
    static long curtime, lasttime, dtime;

    gettimeofday(&tv,NULL);
    curtime = tv.tv_usec/1000;

    dtime = curtime >= lasttime ? curtime - lasttime : (1000 - lasttime) + curtime;
    //LOGD("lasttime: %d curtime: %d dtime: %d\n", lasttime,curtime,dtime);
    lasttime = curtime;
    return dtime;
}

/*
 * frame_lock: used to lock frame process
 * lock frame a moment to show recognize result after a face image has been recognized
 */
static void frame_lock(int time_ms)
{
    capt_op.lock_time_ms = time_ms;
}

static void frame_unlock()
{
    capt_op.lock_time_ms = 0;
}

static int frame_trylock()
{
    if (capt_op.lock_time_ms <= 0) {
        frame_unlock();
        return 0;
    } else {
        capt_op.lock_time_ms -= get_dtime();
    }

    if (capt_op.lock_time_ms <= 0) {
        frame_unlock();
        return 0;
    } else {
        return -1;
    }
}


/*
 * drawer text or png to show some infomation for face recognize
 */

static void draw_text(int x, int y, char* text)
{
    gr_drawer->set_pen_color(0xff, 0, 0);
    gr_drawer->draw_text(x, y, text, 0);
    gr_drawer->display();
}


static void draw_png(uint8_t index)
{
    if (index > 2) {
        LOGE("unknow png index\n");
        return;
    }

    gr_drawer->set_pen_color(0xFF, 0xFF, 0xFF);
    gr_drawer->fill_screen();
    gr_drawer->draw_png(draw_pngs[index].p_surface, draw_pngs[index].pos_x, draw_pngs[index].pos_y);
}


/*
 * save image data to a file
 */
__attribute__((unused)) static int save_image(uint8_t* data, char type)
{
    char filename[64];
    static int filelen, cnt = 0;
    #define FIMENAME    "Test"
    FILE *fp;

    sprintf(filename,"%s%d-%d",FIMENAME,cnt++,type);
    if ((fp = fopen(filename, "wb")) == NULL) {
        LOGE("Can not create BMP file: %s\n", filename);
        return -1;
    }

    filelen = 320*240;
    if (fwrite(data, 1, filelen, fp) != filelen) {
        LOGE("write fail ret: %d\n",filelen);
    }
    fclose(fp);
}

/*
 * extract y value from yuv iamge
 */
static void yuyv_extract_y(uint8_t* yuyvbuf, uint8_t* ybuf, uint32_t width,
                                                   uint32_t height, uint8_t pix_type)
{
    int i, j, k;
    if (pix_type == RECOGNIZE_PIXELS_320X240) {
        // y image: 320 x240
        for (i = 0, j = 0; i < width * height * 2; i += 2, j++) {
            ybuf[j] = yuyvbuf[i];
        }
    } else if (pix_type == RECOGNIZE_PIXELS_240X320) {
        // y image: 240 x320
        for (i = width*2-1, k = 0; i >= 0; --i) {
            if (!(i%2)) {
                for (j = 0; j < height; ++j) {
                    ybuf[k++] = yuyvbuf[j*width*2+i];
                }
            }
        }
    }
}


/*
 * face iamge recognize
 */
static int record_face_feature(uint8_t* pfeature)
{
    int ret;

    ret = NMSDK_IRFACE_AddToFeatureList(face_info.p_list_handle, pfeature, 1);
    if (ret <= 0) {
        LOGE("add to feature list fail\n");
    } else {
        SET_RECOGNIZE_STEP(RECOGNIZE_IDLE);
        LOGI("add to feature list. num: %d\n",ret);
        draw_png(PNG_RECORD_SUCCESSFUL);
        frame_lock(3000);
    }

    SET_RECORD_NUM(ret);
    return ret;
}


static int compare_face_feature(uint8_t* pfeature)
{
    int i,ret;
    uint8_t scores[LIST_MAX_ENTRY];
    ret = NMSDK_IRFACE_CompareFeatureList(face_info.p_list_handle, pfeature, 0, -1, scores);
    if (ret <= 0) {
        LOGE("Compare from feature list fail\n");
    } else {
        SET_RECOGNIZE_STEP(RECOGNIZE_IDLE);
        LOGI("Similarity of compare result: ");
        for (i = 0; i < ret; ++i) {
            printf(" %d", scores[i]);
            if (scores[i] > SIMILARITY_THRESHOLD) {
                break;
            }
        }
        printf("\n");
        if (i < ret) {
            draw_png(PNG_MATCH_SUCCESSFUL);
            frame_lock(3000);
        } else {
            draw_png(PNG_MATCH_FAULT);
            frame_lock(3000);
        }
    }
    return ret;
}

static void image_recognize(uint8_t* yuvbuf, uint32_t width, uint32_t height)
{
    int ret;
    uint8_t *ybuf  = NULL;
    uint32_t  y_width;
    uint32_t  y_height;
    /*
     * // y_xxx image width and height used to face detect and get feature
     */
    if (face_info.recognize_pix == RECOGNIZE_PIXELS_240X320) {
        y_width  = height;
        y_height = width;
    } else {
        y_width  = width;
        y_height = height;
    }

    if (GET_RECOGNIZE_STEP() == RECOGNIZE_IDLE) {
        return;
    }

    ybuf = (uint8_t *)malloc(width * height);
    if (!ybuf) {
        fprintf(stderr, "malloc ybuf failed!!\n");
        return;
    }
    /*
        step:
            1, extect y image from yuyv image
            2, detect face point by y image
            3, generate face feature by y image and the various point in tep 2 got
     */
    // step 1
    yuyv_extract_y(yuvbuf,ybuf,width,height,face_info.recognize_pix);
    // step 2
    ret = NMSDK_IRFACE_Detect(ybuf, y_width, y_height, &face_info.rect, &face_info.pt_left_eye, &face_info.pt_right_eye);
    if (ret < 0) {
        LOGE("face recognize detect fail\n");
    } else if (ret == 0) {
        //LOGE("no face\n");
    } else if (ret == 1) {
        LOGD("face top:%ld, bottom:%ld, left:%ld, right:%ld\n",
            face_info.rect.top, face_info.rect.bottom, face_info.rect.left, face_info.rect.right);
        LOGD("eye letf x:%ld y:%ld, right x:%ld y:%ld\n",
            face_info.pt_left_eye.x, face_info.pt_left_eye.y, face_info.pt_right_eye.x, face_info.pt_right_eye.y);
        // step 3
        ret = NMSDK_IRFACE_Feature(ybuf, y_width, y_height, face_info.pt_left_eye, face_info.pt_right_eye, face_info.p_feature);
        if (ret != 0) {
            LOGE("face recognize feature fail\n");
            ret = -1;
        } else {
            LOGI("face recognize feature successful\n");
            if (GET_RECOGNIZE_STEP() == RECOGNIZE_RECORD) {
                record_face_feature(face_info.p_feature);
            } else if (GET_RECOGNIZE_STEP() == RECOGNIZE_COMPARE) {
                compare_face_feature(face_info.p_feature);
            }
        }
    }
    free(ybuf);
}

static void image_preview(uint8_t* yuvbuf, uint32_t width, uint32_t height)
{
    int ret;
    uint8_t *rgbbuf = NULL;
    struct rgb_pixel_fmt pixel_fmt;

    rgbbuf = (uint8_t *)malloc(width * height * 3);
    if (!rgbbuf) {
        fprintf(stderr, "malloc rgbbuf failed!!\n");
        return;
    }

    ret = cimm->yuv2rgb(yuvbuf, rgbbuf, width, height);
    if (ret < 0) {
        LOGE("yuv 2 rgb fail, errno: %d\n", ret);
        free(rgbbuf);
        return;
    }

    MK_PIXEL_FMT(pixel_fmt);
    cimm->rgb2pixel(rgbbuf, capt_op.ppbuf, width, height, pixel_fmt);
    fbm->display();

    free(rgbbuf);
}


static void frame_receive_cb(uint8_t* buf, uint32_t width, uint32_t height, uint8_t seq)
{
    uint8_t *yuvbuf = buf;
    char info[32];

    if (frame_trylock() != 0)
        return;

    if (seq == capt_op.view_channel) {
        image_preview(yuvbuf, width, height);
        sprintf(info, "%s%s","Current mode: ",step_str[(uint8_t)GET_RECOGNIZE_STEP()]);
        draw_text(0, 0, info);
    } else if (seq == capt_op.recognize_channel) {
        image_recognize(yuvbuf, width, height);
    }
}

static int face_recognize_init()
{
    int feature_size;

    if (NMSDK_IRFACE_Init() != 0) {
        LOGE("face recognize init fail.\n");
        goto err_face_init;
    }
    /*
        creat feature lib
     */
    face_info.p_list_handle = NMSDK_IRFACE_CreateFeatureList(LIST_MAX_ENTRY);
    if (face_info.p_list_handle == NULL) {
        LOGE("face create feature list fail\n");
        goto err_face_op;
    }

    /*
        get size of per feature to malloc memory
     */
    feature_size = NMSDK_IRFACE_FeatureSize();
    face_info.p_feature = (uint8_t*)malloc(feature_size);
    if (face_info.p_feature == NULL) {
        LOGE("face feature malloc fail\n");
        goto err_face_op;
    }

    return 0;

err_face_op:
    NMSDK_IRFACE_Uninit();
err_face_init:
    return -1;
}

static void face_recognize_deinit()
{
    free(face_info.p_feature);
    NMSDK_IRFACE_DestroyFeatureList(face_info.p_list_handle);
    NMSDK_IRFACE_Uninit();
}


static int drawer_init()
{
    char name[32];
    gr_drawer = get_gr_drawer();
    if (gr_drawer->init() < 0) {
        LOGE("Failed to init fb_manager\n");
        goto err_init;
    }

    sprintf(name, "%s", "/res/record-successful.png");
    if (png_decode_image(name, &draw_pngs[0].p_surface) < 0) {
        LOGE("Failed to decode png %s\n", name);
        goto err_decode;
    }

    draw_pngs[0].pos_x = (gr_drawer->get_fb_width()  - draw_pngs[0].p_surface->width)  / 2;
    draw_pngs[0].pos_y = (gr_drawer->get_fb_height() - draw_pngs[0].p_surface->height) / 2;


    sprintf(name, "%s", "/res/match-successful.png");
    if (png_decode_image(name, &draw_pngs[1].p_surface) < 0) {
        LOGE("Failed to decode png %s\n", name);
        goto err_decode;
    }

    draw_pngs[1].pos_x = (gr_drawer->get_fb_width()  - draw_pngs[1].p_surface->width)  / 2;
    draw_pngs[1].pos_y = (gr_drawer->get_fb_height() - draw_pngs[1].p_surface->height) / 2;


    sprintf(name, "%s", "/res/match-fault.png");
    if (png_decode_image(name, &draw_pngs[2].p_surface) < 0) {
        LOGE("Failed to decode png %s\n", name);
        goto err_decode;
    }

    draw_pngs[2].pos_x = (gr_drawer->get_fb_width()  - draw_pngs[2].p_surface->width)  / 2;
    draw_pngs[2].pos_y = (gr_drawer->get_fb_height() - draw_pngs[2].p_surface->height) / 2;

    return 0;

err_decode:
    gr_drawer->deinit();
err_init:
    return -1;
}

static void drawer_deinit()
{
    gr_drawer->deinit();
}

static int fb_init()
{
    fbm = get_fb_manager();

    /*
     * fb manager has been initialized in gr_drawer->init()
     * so need not call init in here, but must to call if it not
     */

    capt_op.ppbuf = (uint16_t *)fbm->get_fbmem();
    if (capt_op.ppbuf == NULL) {
        LOGE("Failed to get fbmem\n");
        goto err_fb_op;
    }

    return 0;

err_fb_op:
    return -1;
}

static void fb_deinit()
{
    /*
     * and need not call fb manager deinit
     */
    return;
}

static int key_input_init()
{
    int ret;

    inputm = get_input_manager();

    ret = inputm->init();
    if (ret < 0) {
        LOGE("Failed to init input manager\n");
        goto err_init;
    }

    ret = inputm->start();
    if (ret < 0) {
        LOGE("Failed to start input manager\n");
        goto err_start;
    }

    inputm->register_event_listener("gpio-keys", input_event_listener);

    return 0;

err_start:
    inputm->deinit();
err_init:
    return -1;
}

static int key_input_deinit()
{
    int ret;

    ret = inputm->stop();
    if (ret < 0) {
        LOGE("Failed to stop input manager\n");
        return -1;
    }

    ret = inputm->deinit();
    if (ret < 0) {
        LOGE("Failed to deinit input manager\n");
        return -1;
    }

    return 0;
}

static void camera_init()
{
    static struct capt_param_t capt_param;

    cimm = get_camera_v4l2_manager();

    capt_param.width          = DEFAULT_WIDTH;
    capt_param.height         = DEFAULT_HEIGHT;
    capt_param.bpp            = DEFAULT_BPP;
    capt_param.nbuf           = DEFAULT_NBUF;
    capt_param.io             = METHOD_MMAP;
    capt_param.fr_cb          = (frame_receive)frame_receive_cb;

    capt_op.view_channel      = CHANNEL_SEQUEUE_COLOR;
    capt_op.recognize_channel = CHANNEL_SEQUEUE_BLACK_WHITE;
    capt_op.lock_time_ms        = 0;


    cimm->init(&capt_param);
    cimm->start();
}

static void camera_deinit()
{
    cimm->stop();
    cimm->deinit();
}

static int set_system_time()
{
    struct tm _tm;
    struct timeval tv;
    time_t timep;

    _tm.tm_sec  = 0;
    _tm.tm_min  = 0;
    _tm.tm_hour = 0;
    _tm.tm_mday = 14;
    _tm.tm_mon  = 7;
    _tm.tm_year = 2017 - 1900;

    timep       = mktime(&_tm);
    tv.tv_sec   = timep;
    tv.tv_usec  = 0;

    if (settimeofday (&tv, (struct timezone *) 0) < 0) {
        LOGE("Set system datatime error!/n");
        return -1;
    }

    return 0;
}

int main(int argc, char const *argv[])
{
    int ret;

    set_system_time();
    ret = face_recognize_init();
    if (ret < 0) {
        goto err_face_recognize;
    }

    ret = drawer_init();
    if (ret < 0) {
        goto err_drawer;
    }

    ret = fb_init();
    if (ret < 0) {
        goto err_fb;
    }

    camera_init();

    ret = key_input_init();
    if (ret < 0) {
        goto err_key_input;
    }

    while(1) {
        sleep(1);
    }

    key_input_deinit();
err_key_input:
    camera_deinit();
    fb_deinit();
err_fb:
    drawer_deinit();
err_drawer:
    face_recognize_deinit();
err_face_recognize:
    return -1;
}