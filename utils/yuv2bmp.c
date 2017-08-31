/*
 *  Copyright (C) 2017, Wang Qiuwei <qiuwei.wang@ingenic.com, panddio@163.com>
 *
 *  Ingenic Linux plarform SDK project
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
#include <utils/yuv2bmp.h>
#include <utils/log.h>
#include <types.h>


#define LOG_TAG                          "yuv2bmp"
/*
 * Functions
 */

static uint32_t yuv2rgb_pixel(int y, int u, int v)
{
    uint32_t rgb_24 = 0;
    uint8_t *pixel = (uint8_t *)&rgb_24;
    int r, g, b;
#if 0
    r = y + (1.370705 * (v - 128));
    g = y - (0.698001 * (v - 128)) - (0.337633 * (u - 128));
    b = y + (1.732446 * (u - 128));
#else
    r = y + ((351 * (v - 128))>>8);
    g = y - ((179 * (v - 128))>>8) - ((86 * (u - 128))>>8);
    b = y + ((444 * (u - 128))>>8);
#endif

    if(r > 255) r = 255;
    if(g > 255) g = 255;
    if(b > 255) b = 255;

    if(r < 0) r = 0;
    if(g < 0) g = 0;
    if(b < 0) b = 0;

    pixel[0] = b;
    pixel[1] = g;
    pixel[2] = r;

    return rgb_24;
}

/*
 * YUV422 to RGB24
 */
int yuv2rgb(uint8_t *yuv, uint8_t *rgb, uint32_t width, uint32_t height)
{
    uint32_t in = 0, out = 0;
    uint32_t pixel_16;
    uint8_t pixel_24[3];
    uint32_t rgb_24;
    uint32_t y0, u, y1, v;

    for(in = 0; in < width * height * 2; in +=4) {
        pixel_16 = yuv[in + 3] << 24 |
                   yuv[in + 2] << 16 |
                   yuv[in + 1] << 8  |
                   yuv[in + 0];
#if 1
        /* Output data mode:Y U Y V */
        y0 = (pixel_16 & 0x000000ff);
        u  = (pixel_16 & 0x0000ff00) >> 8;
        y1 = (pixel_16 & 0x00ff0000) >> 16;
        v  = (pixel_16 & 0xff000000) >> 24;
#elif 0
        /* Output data mode:Y V Y U */
        y0 = (pixel_16 & 0x000000ff);
        v  = (pixel_16 & 0x0000ff00) >> 8;
        y1 = (pixel_16 & 0x00ff0000) >> 16;
        u  = (pixel_16 & 0xff000000) >> 24;
#else
        /* Output data mode:U Y V Y */
        u  = (pixel_16 & 0x000000ff);
        y0 = (pixel_16 & 0x0000ff00) >> 8;
        v  = (pixel_16 & 0x00ff0000) >> 16;
        y1 = (pixel_16 & 0xff000000) >> 24;
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

int rgb2bmp(char *filename, uint32_t width, uint32_t height,\
            int iBitCount, uint8_t *rgbbuf)
{
    uint8_t *dstbuf;
    uint32_t i, j;
    uint64_t linesize, rgbsize;

    // LOGE("sizeof(BITMAPFILEHEADER) = 0x%02x\n", (uint32_t)sizeof(BITMAPFILEHEADER));
    // LOGE("sizeof(BITMAPINFOHEADER) = 0x%02x\n", (uint32_t)sizeof(BITMAPINFOHEADER));

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
        dstbuf = (uint8_t *)malloc(rgbsize);
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
