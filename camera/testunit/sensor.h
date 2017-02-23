/*************************************************************************
	> Filename: sensor.h
	>   Author: qiuwei.wang
	>    Email: qiuwei.wang@ingenic.com / panddio@163.com
	> Datatime: Tue 21 Feb 2017 03:02:27 PM CST
 ************************************************************************/

#ifndef _SENSOR_H
#define _SENSOR_H

enum sensor_list {
    GC2155,
    BF3703,
};

struct sensor_info {
    enum sensor_list sensor;
    char *name;
};

enum image_width {
	W_QVGA	= 320,
	W_VGA	= 640,
	W_720P  = 1280,
};

enum image_height {
	H_QVGA	= 240,
	H_VGA	= 480,
	H_720P  = 720,
};

struct image_fmt {
	char *name;
	enum image_width width;
	enum image_height height;
	const struct camera_regval_list *regs_val;
};

#define IMAGE_FMT(n, w, h, r) \
	{.name = n, .width = w , .height = h, .regs_val = r }

/* Select the nearest higher resolution */
const struct image_fmt *select_image_fmt(uint32_t *width, uint32_t *height,
                                         const struct image_fmt supported_image_fmts[])
{
	int index = 0;
    while(supported_image_fmts[index].name &&
          supported_image_fmts[index].regs_val) {
        if (supported_image_fmts[index].width  >= *width &&
            supported_image_fmts[index].height >= *height) {
            *width = supported_image_fmts[index].width;
            *height = supported_image_fmts[index].height;
            return &supported_image_fmts[index];
        }

        index++;
    }

    *width = supported_image_fmts[0].width;
    *height = supported_image_fmts[0].height;
    return &supported_image_fmts[0];
}

#endif /* _SENSOR_H */
