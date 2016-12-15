/*************************************************************************
	> Filename: yuv2bmp.h
	>   Author: Wang Qiuwei
	>    Email: qiuwei.wang@ingenic.com / panddio@163.com
	> Datatime: Thu 15 Dec 2016 09:06:00 AM CST
 ************************************************************************/

#ifndef _BMP_H
#define _BMP_H

#define FAR
#define DWORD unsigned int
#define WORD  unsigned short
#ifndef LONG
#define LONG  unsigned long
#endif
#define BYTE  unsigned char

#ifndef PACKED
#define PACKED
#endif
#ifndef GCC_PACKED
#define GCC_PACKED __attribute__((packed))
#endif

/* structures for defining DIBs */
typedef PACKED struct tagBITMAPCOREHEADER
{
	DWORD   bcSize;                 /* used to get to color table */
	WORD    bcWidth;
	WORD    bcHeight;
	WORD    bcPlanes;
	WORD    bcBitCount;
} GCC_PACKED BITMAPCOREHEADER, FAR *LPBITMAPCOREHEADER, *PBITMAPCOREHEADER;

typedef PACKED struct tagBITMAPINFOHEADER
{
	DWORD      biSize;
	LONG       biWidth;
	LONG       biHeight;
	WORD       biPlanes;
	WORD       biBitCount;
	DWORD      biCompression;
	DWORD      biSizeImage;
	LONG       biXPelsPerMeter;
	LONG       biYPelsPerMeter;
	DWORD      biClrUsed;
	DWORD      biClrImportant;
} GCC_PACKED BITMAPINFOHEADER, FAR *LPBITMAPINFOHEADER, *PBITMAPINFOHEADER;

typedef PACKED struct tagRGBQUAD
{
	BYTE    rgbBlue;
	BYTE    rgbGreen;
	BYTE    rgbRed;
	BYTE    rgbReserved;
} GCC_PACKED RGBQUAD;
typedef RGBQUAD FAR* LPRGBQUAD;

typedef PACKED struct tagRGBTRIPLE
{
	BYTE    rgbtBlue;
	BYTE    rgbtGreen;
	BYTE    rgbtRed;
} GCC_PACKED RGBTRIPLE;


typedef PACKED struct tagBITMAPINFO
{
	BITMAPINFOHEADER    bmiHeader;
	RGBQUAD             bmiColors[1];
} GCC_PACKED BITMAPINFO, FAR *LPBITMAPINFO, *PBITMAPINFO;

typedef PACKED struct tagBITMAPCOREINFO
{
	BITMAPCOREHEADER    bmciHeader;
	RGBTRIPLE           bmciColors[1];
} GCC_PACKED BITMAPCOREINFO, FAR *LPBITMAPCOREINFO, *PBITMAPCOREINFO;

typedef PACKED struct tagBITMAPFILEHEADER
{
	WORD    bfType;
	DWORD   bfSize;
	WORD    bfReserved1;
	WORD    bfReserved2;
	DWORD   bfOffBits;
}GCC_PACKED BITMAPFILEHEADER, FAR *LPBITMAPFILEHEADER, *PBITMAPFILEHEADER;

/*
 * Extern functions
 */
int yuv2rgb(unsigned char *yuv, unsigned char *rgb, unsigned int width, unsigned height);
int rgb2bmp(char *filename, unsigned int width, unsigned int height, int iBitCount, unsigned char *rgbbuf);

#endif /* _BMP_H */
