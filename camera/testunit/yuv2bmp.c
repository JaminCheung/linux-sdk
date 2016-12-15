/*************************************************************************
	> Filename: yuv2bmp.c
	>   Author: Wang Qiuwei
	>    Email: qiuwei.wang@ingenic.com / panddio@163.com
	> Datatime: Thu 15 Dec 2016 09:08:44 AM CST
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#include <utils/log.h>
#include <utils/assert.h>
#include <camera/camera_manager.h>

#include "yuv2bmp.h"


#define LOG_TAG  "yuv2bmp"

/*
 * Functions
 */

static unsigned int yuv2rgb_pixel(int y, int u, int v)
{
	unsigned int rgb_24 = 0;
	unsigned char *pixel = (unsigned char *)&rgb_24;
	int r, g, b;

	r = y + (1.370705 * (v - 128));
	g = y - (0.698001 * (v - 128)) - (0.337633 * (u - 128));
	b = y + (1.732446 * (u - 128));

	if(r > 255) r = 255;
	if(g > 255) g = 255;
	if(b > 255) b = 255;

    if(r < 0) r = 0;
    if(g < 0) g = 0;
    if(b < 0) b = 0;

	pixel[0] = r;
	pixel[1] = g;
	pixel[2] = b;

	return rgb_24;
}

/*
 * YUV422 to RGB24
 */
int yuv2rgb(unsigned char *yuv, unsigned char *rgb, unsigned int width, unsigned height)
{
	unsigned int in = 0, out = 0;
	unsigned int pixel_16;
	unsigned char pixel_24[3];
	unsigned int rgb_24;
	unsigned int y0, u, y1, v;

	for(in = 0; in < width * height * 2; in +=4) {
		pixel_16 = yuv[in + 3] << 24 |
				   yuv[in + 2] << 16 |
				   yuv[in + 1] << 8  |
				   yuv[in + 0];
#if 0
		u  = (pixel_16 & 0x000000ff);
		y0 = (pixel_16 & 0x0000ff00) >> 8;
		v  = (pixel_16 & 0x00ff0000) >> 16;
		y1 = (pixel_16 & 0xff000000) >> 24;
#else
		y0 = (pixel_16 & 0x000000ff);
		v  = (pixel_16 & 0x0000ff00) >> 8;
		y1 = (pixel_16 & 0x00ff0000) >> 16;
		u  = (pixel_16 & 0xff000000) >> 24;
#endif

		rgb_24 = yuv2rgb_pixel(y0, u, v);

		pixel_24[0] = (rgb_24 & 0x000000ff);
		pixel_24[1] = (rgb_24 & 0x0000ff00) >> 8;
		pixel_24[2] = (rgb_24 & 0x00ff0000) >> 16;

		rgb[out++] = pixel_24[0];
		rgb[out++] = pixel_24[1];
		rgb[out++] = pixel_24[2];

		rgb_24 = yuv2rgb_pixel(y1, u, v);

		pixel_24[0] = (rgb_24 & 0x000000ff);
		pixel_24[1] = (rgb_24 & 0x0000ff00) >> 8;
		pixel_24[2] = (rgb_24 & 0x00ff0000) >> 16;

		rgb[out++] = pixel_24[0];
		rgb[out++] = pixel_24[1];
		rgb[out++] = pixel_24[2];
	}

	return 0;
}

int rgb2bmp(char *filename, unsigned int width, unsigned int height,\
        int iBitCount, unsigned char *rgbbuf)
{
    unsigned char *dstbuf;
    unsigned int i, j;
	unsigned long linesize, rgbsize;

    LOGD("sizeof(BITMAPFILEHEADER) = 0x%02x\n", (unsigned int)sizeof(BITMAPFILEHEADER));
    LOGD("sizeof(BITMAPINFOHEADER) = 0x%02x\n", (unsigned int)sizeof(BITMAPINFOHEADER));

	if(iBitCount == 24) {
		FILE *fp;
		long ret;
		BITMAPFILEHEADER bmpHeader;
		BITMAPINFO bmpInfo;

		if((fp = fopen(filename, "wb")) == NULL) {
			LOGE("Can not create BMP file: %s\n", filename);
			return -1;
		}

		rgbsize = width * height * 3;

		bmpHeader.bfType = (WORD)(('M' << 8) | 'B');
		bmpHeader.bfSize = rgbsize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
		bmpHeader.bfReserved1 = 0;
		bmpHeader.bfReserved2 = 0;
		bmpHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

		bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmpInfo.bmiHeader.biWidth = width;
		bmpInfo.bmiHeader.biHeight = height;
		bmpInfo.bmiHeader.biPlanes = 1;
		bmpInfo.bmiHeader.biBitCount = iBitCount;
		bmpInfo.bmiHeader.biCompression = 0;
        bmpInfo.bmiHeader.biSizeImage = rgbsize;
		bmpInfo.bmiHeader.biXPelsPerMeter = 0;
		bmpInfo.bmiHeader.biYPelsPerMeter = 0;
		bmpInfo.bmiHeader.biClrUsed = 0;
		bmpInfo.bmiHeader.biClrImportant = 0;

		if((ret = fwrite(&bmpHeader, 1, sizeof(BITMAPFILEHEADER), fp))
				!= sizeof(BITMAPFILEHEADER))
			LOGE("write BMP file header failed: ret=%ld\n", ret);

		if((ret = fwrite(&(bmpInfo.bmiHeader), 1, sizeof(BITMAPINFOHEADER), fp))
				!= sizeof(BITMAPINFOHEADER))
			LOGE("write BMP file info failed: ret=%ld\n", ret);

        /* convert rgbbuf */
        dstbuf = (unsigned char *)malloc(rgbsize);
        if(!dstbuf) {
            LOGE("malloc dstbuf failed !!\n");
            return -1;
        }

        linesize = width * 3; //line size
        for(i = 0, j = height -1; i < height - 1; i++, j--)
            memcpy((dstbuf + (linesize * j)), (rgbbuf + (linesize * i)), linesize);

		if((ret = fwrite(dstbuf, 1, rgbsize, fp)) != rgbsize)
			LOGE("write BMP file date failed: ret=%ld\n", ret);

        free(dstbuf);
		fclose(fp);
		return 0;
	} else {
        LOGE("%s: error iBitCount != 24\n", __FUNCTION__);
        return -1;
    }
}
