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
#include <fb/fb_manager.h>
#include <camera_v4l2/camera_v4l2_manager.h>

/*
 * Macro
 */

typedef enum {
    CHANNEL_SEQUEUE_COLOR             = 0x00,
    CHANNEL_SEQUEUE_BLACK_WHITE       = 0x01
} chselect_m;


typedef enum {
    CAPTURE_PICTURE = 0,
    PREVIEW_PICTURE,
} action_m;

#define DEFAULT_WIDTH                    320
#define DEFAULT_HEIGHT                   240
#define DEFAULT_BPP                      16
#define DEFAULT_NBUF                     1
#define DEFAULT_ACTION   				 PREVIEW_PICTURE
#define DEFAULT_DOUBLE_CHANNEL           0
#define DEFAULT_CHANNEL                  CHANNEL_SEQUEUE_COLOR



#define LOG_TAG                         "camera_v4l2"

struct camera_v4l2_manager* cimm;
struct fb_manager* fbm;

struct _capt_op{
    action_m action;            // action capture or preview
    chselect_m channel;         // select operate ch for action
    char double_ch;             // channel num
    char* filename;             // picture save name when action is capture picture
    short *ppbuf;               // map lcd piexl buf
}capt_op;

static const char short_options[] = "c:phmn:rux:y:ds:";

static const struct option long_options[] = {
    { "help",       0,      NULL,           'h' },
    { "mmap",       0,      NULL,           'm' },
    { "nbuf",       1,      NULL,           'n' },
    { "read",       0,      NULL,           'r' },
    { "userp",      0,      NULL,           'u' },
    { "width",      1,      NULL,           'x' },
    { "height",     1,      NULL,           'y' },
    { "capture",    1,      NULL,           'c' },
    { "preview",    0,      NULL,           'p' },
    { "double",     0,      NULL,           'd' },
    { "select",     1,      NULL,           's' },
    { 0, 0, 0, 0 }
};

/*
 * Functions
 */
static void usage(FILE *fp, int argc, char *argv[])
{
    fprintf(fp,
             "\nUsage: %s [options]\n"
             "Options:\n"
             "-h | --help          Print this message\n"
             "-m | --mmap          Use memory mapped buffers\n"
             "-n | --nbuf          Request buffer numbers\n"
             "-r | --read          Use read() calls\n"
             "-u | --userp         Use application allocated buffers\n"
             "-x | --width         Capture width\n"
             "-y | --height        Capture height\n"
             "-c | --capture       Take picture\n"
             "-p | --preview       Preview picture to LCD\n"
             "-d | --double        Used double camera sensor\n"
             "-s | --select        Select operate channel, 0(color) or 1(black&white)\n"
             "\n", argv[0]);
}


static void frame_receive_cb(unsigned char* buf, unsigned int width, unsigned int height, unsigned char seq)
{
    int ret;
    unsigned char *yuvbuf = buf;
    unsigned char *rgbbuf = NULL;
    int monkt;
    //return;
    rgbbuf = (unsigned char *)malloc(width * height * 3);
    if (!rgbbuf) {
        fprintf(stderr, "malloc rgbbuf failed!!\n");
        return;
    }


    if (capt_op.double_ch != 1) {
        seq = capt_op.channel;
    }

    if (seq == capt_op.channel)
    {
        ret = cimm->yuv2rgb(yuvbuf, rgbbuf, width, height);
        if (ret < 0){
            printf("yuv 2 rgb fail, errno: %d\n", ret);
            free(rgbbuf);
            return;
        }

        if (capt_op.action == CAPTURE_PICTURE){
            ret = cimm->build_bmp(rgbbuf, width, height, capt_op.filename);
            if (ret < 0){
                printf("make bmp picutre fail, errno: %d\n",ret);
            }
        } else if (capt_op.action == PREVIEW_PICTURE) {
            cimm->rgb2pixel(rgbbuf, capt_op.ppbuf, width, height);
            fbm->display();
        }
    }

    free(rgbbuf);
}

int main(int argc, char *argv[])
{
    struct capt_param_t capt_param;

    capt_param.width  = DEFAULT_WIDTH;
    capt_param.height = DEFAULT_HEIGHT;
    capt_param.bpp    = DEFAULT_BPP;
    capt_param.nbuf   = DEFAULT_NBUF;
    capt_param.io     = METHOD_MMAP;
    capt_param.fr_cb  = frame_receive_cb;

    capt_op.double_ch = DEFAULT_DOUBLE_CHANNEL;
    capt_op.channel   = DEFAULT_CHANNEL;
    capt_op.action    = DEFAULT_ACTION;

    while(1) {
        int oc;

        oc = getopt_long(argc, argv, \
                         short_options, long_options, \
                         NULL);
        if(-1 == oc)
            break;

        switch(oc) {
        case 0:
            break;

        case 'h':
            usage(stdout, argc, argv);
            exit(EXIT_SUCCESS);
            break;

        case 'm':
            capt_param.io = METHOD_MMAP;
            break;

        case 'r':
            capt_param.io = METHOD_READ;
            break;

        case 'u':
            capt_param.io = METHOD_USERPTR;
            break;

        case 'n':
            capt_param.nbuf = atoi(optarg);
            break;

        case 'x':
            capt_param.width = atoi(optarg);
            break;

        case 'y':
            capt_param.height = atoi(optarg);
            break;

        case 'c':
            capt_op.action = CAPTURE_PICTURE;
            capt_op.filename = optarg;
            printf(" %s ---\n", capt_op.filename);
            break;

        case 'p':
            capt_op.action = PREVIEW_PICTURE;
            break;

        case 'd':
            capt_op.double_ch = 1;
            break;

        case 's':
            capt_op.channel = atoi(optarg);
            break;
        default:
            usage(stderr, argc, argv);
            exit(EXIT_FAILURE);
            break;
        }
    }

    fbm = get_fb_manager();

    if (fbm->init() < 0) {
        printf("Failed to init fb_manager\n");
        exit(EXIT_FAILURE);
    }

    capt_op.ppbuf = (uint16_t *)fbm->get_fbmem();
    if (capt_op.ppbuf == NULL) {
        printf("Failed to get fbmem\n");
        exit(EXIT_FAILURE);
    }

    cimm = get_camera_v4l2_manager();

    cimm->init(&capt_param);
    cimm->start();
    if (capt_op.action == PREVIEW_PICTURE){
        while(1);
    } else if (capt_op.action == CAPTURE_PICTURE) {
        sleep(1);
    }
    cimm->stop();
    cimm->deinit();
    return 0;
}