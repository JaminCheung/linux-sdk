#ifndef __GC2155_H__
#define __GC2155_H__

static const struct camera_regval_list gc2155_init_regs[] = {
    {0xfe,0xf0},
    {0xfe,0xf0},
    {0xfe,0xf0},
    {0xfc,0x06},
    {0xf6,0x00},
    {0xf7,0x1d},
    {0xf8,0x84},
    {0xfa,0x00},
    {0xf9,0xfe},
    {0xf2,0x00},
    {0xfe,0x00},
    {0x03,0x04},
    {0x04,0xe2},
    {0x09,0x00},
    {0x0a,0x00},
    {0x0b,0x00},
    {0x0c,0x00},
    {0x0d,0x04},
    {0x0e,0xc0},
    {0x0f,0x06},
    {0x10,0x50},
    {0x12,0x2e},
    {0x17,0x14}, // mirror
    {0x18,0x02},
    {0x19,0x0e},
    {0x1a,0x01},
    {0x1b,0x4b},
    {0x1c,0x07},
    {0x1d,0x10},
    {0x1e,0x98},
    {0x1f,0x78},
    {0x20,0x05},
    {0x21,0x40},
    {0x22,0xf0},
    {0x24,0x16},
    {0x25,0x01},
    {0x26,0x10},
    {0x2d,0x40},
    {0x30,0x01},
    {0x31,0x90},
    {0x33,0x04},
    {0x34,0x01},
    {0xfe,0x00},
    {0x80,0xff},
    {0x81,0x2c},
    {0x82,0xfa},
    {0x83,0x00},
    {0x84,0x02}, //y u yv
    {0x85,0x08},
    {0x86,0x02},
    {0x89,0x03},
    {0x8a,0x00},
    {0x8b,0x00},
    {0xb0,0x55},
    {0xc3,0x11}, //00
    {0xc4,0x20},
    {0xc5,0x30},
    {0xc6,0x38},
    {0xc7,0x40},
    {0xec,0x02},
    {0xed,0x04},
    {0xee,0x60},
    {0xef,0x90},
    {0xb6,0x01},
    {0x90,0x01},
    {0x91,0x00},
    {0x92,0x00},
    {0x93,0x00},
    {0x94,0x00},
    {0x95,0x04},
    {0x96,0xb0},
    {0x97,0x06},
    {0x98,0x40},
    {0xfe,0x00},
    {0x18,0x02},
    {0x40,0x42},
    {0x41,0x00},
    {0x43,0x5b},//0X54
    {0x5e,0x00},
    {0x5f,0x00},
    {0x60,0x00},
    {0x61,0x00},
    {0x62,0x00},
    {0x63,0x00},
    {0x64,0x00},
    {0x65,0x00},
    {0x66,0x20},
    {0x67,0x20},
    {0x68,0x20},
    {0x69,0x20},
    {0x6a,0x08},
    {0x6b,0x08},
    {0x6c,0x08},
    {0x6d,0x08},
    {0x6e,0x08},
    {0x6f,0x08},
    {0x70,0x08},
    {0x71,0x08},
    {0x72,0xf0},
    {0x7e,0x3c},
    {0x7f,0x00},
    {0xfe,0x00},
    {0xfe,0x01},
    {0x01,0x08},
    {0x02,0xc0},
    {0x03,0x04},
    {0x04,0x90},
    {0x05,0x30},
    {0x06,0x98},
    {0x07,0x28},
    {0x08,0x6c},
    {0x09,0x00},
    {0x0a,0xc2},
    {0x0b,0x11},
    {0x0c,0x10},
    {0x13,0x2d},
    {0x17,0x00},
    {0x1c,0x11},
    {0x1e,0x61},
    {0x1f,0x30},
    {0x20,0x40},
    {0x22,0x80},
    {0x23,0x20},
    {0x12,0x35},
    {0x15,0x50},
    {0x10,0x31},
    {0x3e,0x28},
    {0x3f,0xe0},
    {0x40,0xe0},
    {0x41,0x08},
    {0xfe,0x02},
    {0x0f,0x05},
    {0xfe,0x02},
    {0x90,0x6c},
    {0x91,0x03},
    {0x92,0xc4},
    {0x97,0x64},
    {0x98,0x88},
    {0x9d,0x08},
    {0xa2,0x11},
    {0xfe,0x00},
    {0xfe,0x02},
    {0x80,0xc1},
    {0x81,0x08},
    {0x82,0x05},
    {0x83,0x04},
    {0x86,0x80},
    {0x87,0x30},
    {0x88,0x15},
    {0x89,0x80},
    {0x8a,0x60},
    {0x8b,0x30},
    {0xfe,0x01},
    {0x21,0x14},
    {0xfe,0x02},
    {0x3c,0x06},
    {0x3d,0x40},
    {0x48,0x30},
    {0x49,0x06},
    {0x4b,0x08},
    {0x4c,0x20},
    {0xa3,0x50},
    {0xa4,0x30},
    {0xa5,0x40},
    {0xa6,0x80},
    {0xab,0x40},
    {0xae,0x0c},
    {0xb3,0x42},
    {0xb4,0x24},
    {0xb6,0x50},
    {0xb7,0x01},
    {0xb9,0x28},
    {0xfe,0x00},
    {0xfe,0x02},
    {0x10,0x0d},
    {0x11,0x12},
    {0x12,0x17},
    {0x13,0x1c},
    {0x14,0x27},
    {0x15,0x34},
    {0x16,0x44},
    {0x17,0x55},
    {0x18,0x6e},
    {0x19,0x81},
    {0x1a,0x91},
    {0x1b,0x9c},
    {0x1c,0xaa},
    {0x1d,0xbb},
    {0x1e,0xca},
    {0x1f,0xd5},
    {0x20,0xe0},
    {0x21,0xe7},
    {0x22,0xed},
    {0x23,0xf6},
    {0x24,0xfb},
    {0x25,0xff},
    {0xfe,0x02},
    {0x26,0x0d},
    {0x27,0x12},
    {0x28,0x17},
    {0x29,0x1c},
    {0x2a,0x27},
    {0x2b,0x34},
    {0x2c,0x44},
    {0x2d,0x55},
    {0x2e,0x6e},
    {0x2f,0x81},
    {0x30,0x91},
    {0x31,0x9c},
    {0x32,0xaa},
    {0x33,0xbb},
    {0x34,0xca},
    {0x35,0xd5},
    {0x36,0xe0},
    {0x37,0xe7},
    {0x38,0xed},
    {0x39,0xf6},
    {0x3a,0xfb},
    {0x3b,0xff},
    {0xfe,0x02},
    {0xd1,0x28},
    {0xd2,0x28},
    {0xdd,0x14},
    {0xde,0x88},
    {0xed,0x80},
    {0xfe,0x01},
    {0xc2,0x1f},
    {0xc3,0x13},
    {0xc4,0x0e},
    {0xc8,0x16},
    {0xc9,0x0f},
    {0xca,0x0c},
    {0xbc,0x52},
    {0xbd,0x2c},
    {0xbe,0x27},
    {0xb6,0x47},
    {0xb7,0x32},
    {0xb8,0x30},
    {0xc5,0x00},
    {0xc6,0x00},
    {0xc7,0x00},
    {0xcb,0x00},
    {0xcc,0x00},
    {0xcd,0x00},
    {0xbf,0x0e},
    {0xc0,0x00},
    {0xc1,0x00},
    {0xb9,0x08},
    {0xba,0x00},
    {0xbb,0x00},
    {0xaa,0x0a},
    {0xab,0x0c},
    {0xac,0x0d},
    {0xad,0x02},
    {0xae,0x06},
    {0xaf,0x05},
    {0xb0,0x00},
    {0xb1,0x05},
    {0xb2,0x02},
    {0xb3,0x04},
    {0xb4,0x04},
    {0xb5,0x05},
    {0xd0,0x00},
    {0xd1,0x00},
    {0xd2,0x00},
    {0xd6,0x02},
    {0xd7,0x00},
    {0xd8,0x00},
    {0xd9,0x00},
    {0xda,0x00},
    {0xdb,0x00},
    {0xd3,0x00},
    {0xd4,0x00},
    {0xd5,0x00},
    {0xa4,0x04},
    {0xa5,0x00},
    {0xa6,0x77},
    {0xa7,0x77},
    {0xa8,0x77},
    {0xa9,0x77},
    {0xa1,0x80},
    {0xa2,0x80},
    {0xfe,0x01},
    {0xdc,0x35},
    {0xdd,0x28},
    {0xdf,0x0d},
    {0xe0,0x70},
    {0xe1,0x78},
    {0xe2,0x70},
    {0xe3,0x78},
    {0xe6,0x90},
    {0xe7,0x70},
    {0xe8,0x90},
    {0xe9,0x70},
    {0xfe,0x00},
    {0xfe,0x01},
    {0x4f,0x00},
    {0x4f,0x00},
    {0x4b,0x01},
    {0x4f,0x00},
    {0x4c,0x01},
    {0x4d,0x71},
    {0x4e,0x02},
    {0x4c,0x01},
    {0x4d,0x91},
    {0x4e,0x02},
    {0x4c,0x01},
    {0x4d,0x50},
    {0x4e,0x02},
    {0x4c,0x01},
    {0x4d,0x70},
    {0x4e,0x02},
    {0x4c,0x01},
    {0x4d,0x90},
    {0x4e,0x02},
    {0x4c,0x01},
    {0x4d,0xb0},
    {0x4e,0x02},
    {0x4c,0x01},
    {0x4d,0xd0},
    {0x4e,0x02},
    {0x4c,0x01},
    {0x4d,0x4f},
    {0x4e,0x02},
    {0x4c,0x01},
    {0x4d,0x6f},
    {0x4e,0x02},
    {0x4c,0x01},
    {0x4d,0x8f},
    {0x4e,0x02},
    {0x4c,0x01},
    {0x4d,0xaf},
    {0x4e,0x02},
    {0x4c,0x01},
    {0x4d,0xcf},
    {0x4e,0x02},
    {0x4c,0x01},
    {0x4d,0x6e},
    {0x4e,0x03},
    {0x4c,0x01},
    {0x4d,0x8e},
    {0x4e,0x03},
    {0x4c,0x01},
    {0x4d,0xae},
    {0x4e,0x03},
    {0x4c,0x01},
    {0x4d,0xce},
    {0x4e,0x03},
    {0x4c,0x01},
    {0x4d,0x4d},
    {0x4e,0x03},
    {0x4c,0x01},
    {0x4d,0x6d},
    {0x4e,0x03},
    {0x4c,0x01},
    {0x4d,0x8d},
    {0x4e,0x03},
    {0x4c,0x01},
    {0x4d,0xad},
    {0x4e,0x03},
    {0x4c,0x01},
    {0x4d,0xcd},
    {0x4e,0x03},
    {0x4c,0x01},
    {0x4d,0x4c},
    {0x4e,0x03},
    {0x4c,0x01},
    {0x4d,0x6c},
    {0x4e,0x03},
    {0x4c,0x01},
    {0x4d,0x8c},
    {0x4e,0x03},
    {0x4c,0x01},
    {0x4d,0xac},
    {0x4e,0x03},
    {0x4c,0x01},
    {0x4d,0xcc},
    {0x4e,0x03},
    {0x4c,0x01},
    {0x4d,0xec},
    {0x4e,0x03},
    {0x4c,0x01},
    {0x4d,0x4b},
    {0x4e,0x03},
    {0x4c,0x01},
    {0x4d,0x6b},
    {0x4e,0x03},
    {0x4c,0x01},
    {0x4d,0x8b},
    {0x4e,0x03},
    {0x4c,0x01},
    {0x4d,0xab},
    {0x4e,0x03},
    {0x4c,0x01},
    {0x4d,0x8a},
    {0x4e,0x04},
    {0x4c,0x01},
    {0x4d,0xaa},
    {0x4e,0x04},
    {0x4c,0x01},
    {0x4d,0xca},
    {0x4e,0x04},
    {0x4c,0x01},
    {0x4d,0xa9},
    {0x4e,0x04},
    {0x4c,0x01},
    {0x4d,0xc9},
    {0x4e,0x04},
    {0x4c,0x01},
    {0x4d,0xcb},
    {0x4e,0x05},
    {0x4c,0x01},
    {0x4d,0xeb},
    {0x4e,0x05},
    {0x4c,0x02},
    {0x4d,0x0b},
    {0x4e,0x05},
    {0x4c,0x02},
    {0x4d,0x2b},
    {0x4e,0x05},
    {0x4c,0x02},
    {0x4d,0x4b},
    {0x4e,0x05},
    {0x4c,0x01},
    {0x4d,0xea},
    {0x4e,0x05},
    {0x4c,0x02},
    {0x4d,0x0a},
    {0x4e,0x05},
    {0x4c,0x02},
    {0x4d,0x2a},
    {0x4e,0x05},
    {0x4c,0x02},
    {0x4d,0x6a},
    {0x4e,0x06},
    {0x4c,0x02},
    {0x4d,0x29},
    {0x4e,0x06},
    {0x4c,0x02},
    {0x4d,0x49},
    {0x4e,0x06},
    {0x4c,0x02},
    {0x4d,0x69},
    {0x4e,0x06},
    {0x4c,0x02},
    {0x4d,0x89},
    {0x4e,0x06},
    {0x4c,0x02},
    {0x4d,0xa9},
    {0x4e,0x06},
    {0x4c,0x02},
    {0x4d,0xc9},
    {0x4e,0x06},
    {0x4c,0x02},
    {0x4d,0x48},
    {0x4e,0x06},
    {0x4c,0x02},
    {0x4d,0x68},
    {0x4e,0x06},
    {0x4c,0x03},
    {0x4d,0x09},
    {0x4e,0x07},
    {0x4c,0x02},
    {0x4d,0xa8},
    {0x4e,0x07},
    {0x4c,0x02},
    {0x4d,0xc8},
    {0x4e,0x07},
    {0x4c,0x02},
    {0x4d,0xe8},
    {0x4e,0x07},
    {0x4c,0x03},
    {0x4d,0x08},
    {0x4e,0x07},
    {0x4c,0x03},
    {0x4d,0x28},
    {0x4e,0x07},
    {0x4c,0x02},
    {0x4d,0x87},
    {0x4e,0x07},
    {0x4c,0x02},
    {0x4d,0xa7},
    {0x4e,0x07},
    {0x4c,0x02},
    {0x4d,0xc7},
    {0x4e,0x07},
    {0x4c,0x02},
    {0x4d,0xe7},
    {0x4e,0x07},
    {0x4c,0x03},
    {0x4d,0x07},
    {0x4e,0x07},
    {0x4f,0x01},
    {0xfe,0x01},
    {0x50,0x80},
    {0x51,0xa8},
    {0x52,0x57},
    {0x53,0x38},
    {0x54,0xc7},
    {0x56,0x0e},
    {0x58,0x08},
    {0x5b,0x00},
    {0x5c,0x74},
    {0x5d,0x8b},
    {0x61,0xd3},
    {0x62,0x90},
    {0x63,0xaa},
    {0x65,0x04},
    {0x67,0xb2},
    {0x68,0xac},
    {0x69,0x00},
    {0x6a,0xb2},
    {0x6b,0xac},
    {0x6c,0xdc},
    {0x6d,0xb0},
    {0x6e,0x30},
    {0x6f,0x40},
    {0x70,0x05},
    {0x71,0x80},
    {0x72,0x80},
    {0x73,0x30},
    {0x74,0x01},
    {0x75,0x01},
    {0x7f,0x08},
    {0x76,0x70},
    {0x77,0x48},
    {0x78,0xa0},
    {0xfe,0x00},
    {0xfe,0x02},
    {0xc0,0x01},
    {0xc1,0x4a},
    {0xc2,0xf3},
    {0xc3,0xfc},
    {0xc4,0xe4},
    {0xc5,0x48},
    {0xc6,0xec},
    {0xc7,0x45},
    {0xc8,0xf8},
    {0xc9,0x02},
    {0xca,0xfe},
    {0xcb,0x42},
    {0xcc,0x00},
    {0xcd,0x45},
    {0xce,0xf0},
    {0xcf,0x00},
    {0xe3,0xf0},
    {0xe4,0x45},
    {0xe5,0xe8},
    {0xfe,0x01},
    {0x9f,0x42},
    {0xfe,0x00},
    {0xfe,0x00},
    {0xf2,0x0f},
    {0xfe,0x00},
    {0x05,0x01},
    {0x06,0x56},
    {0x07,0x00},
    {0x08,0x32},
    {0xfe,0x01},
    {0x25,0x00},
    {0x26,0xfa},
    {0x27,0x04},
    {0x28,0xe2}, //20fps
    {0x29,0x06},
    {0x2a,0xd6}, //16fps
    {0x2b,0x07},
    {0x2c,0xd0}, //12fps
    {0x2d,0x0b},
    {0x2e,0xb8}, //8fps
    {0xfe,0x00},
    {0xfe,0x00},
    {0xfa,0x00},
    {0xfd,0x01},
    {0xfe,0x00},
    {0x90,0x01},
    {0x91,0x00},
    {0x92,0x00},
    {0x93,0x00},
    {0x94,0x00},

#if 1
    {0x95,0x00},// win_size 320 * 240
    {0x96,0xf0},
    {0x97,0x01},
    {0x98,0x40},
#endif

    {0x99,0x11}, //
    {0x9a,0x06},
    {0xfe,0x01},
    {0xec,0x01},
    {0xed,0x02},
    {0xee,0x30},
    {0xef,0x48},
    {0xfe,0x01},
    {0x74,0x00},
    {0xfe,0x01},
    {0x01,0x04},
    {0x02,0x60},
    {0x03,0x02},
    {0x04,0x48},
    {0x05,0x18},
    {0x06,0x4c},
    {0x07,0x14},
    {0x08,0x36},
    {0x0a,0xc0},
    {0x21,0x14},
    {0xfe,0x00},
    {0xfe,0x00},
    {0xc3,0x11},
    {0xc4,0x20},
    {0xc5,0x30},
    {0xfa,0x11},//pclk rate
    {0x86,0x06},//pclk polar
    {0xfe,0x00},

    {0x95,0x00},// win_size 320 * 240
    {0x96,0xf0},
    {0x97,0x01},
    {0x98,0x40},

    {0x95,0x04},// win_size 1600 * 1200
    {0x96,0xb0},
    {0x97,0x06},
    {0x98,0x40},

    {0x95,0x01},// win_size 640 * 480
    {0x96,0xe0},
    {0x97,0x02},
    {0x98,0x80},

    ENDMARKER,
};

static const struct camera_regval_list gc2155_sensor_2M_regs[] = {
    {0xfe, 0x00},
    {0xfa, 0x11},
    {0xfd, 0x00},
// crop window
    {0xfe, 0x00},
    {0x90, 0x01},
    {0x91, 0x00},
    {0x92, 0x00},
    {0x93, 0x00},
    {0x94, 0x00},
    {0x95, 0x04},
    {0x96, 0xb0},
    {0x97, 0x06},
    {0x98, 0x40},
    {0x99, 0x11},
    {0x9a, 0x06},
// AWB
    {0xfe, 0x00},
    {0xec, 0x02},
    {0xed, 0x04},
    {0xee, 0x60},
    {0xef, 0x90},
    {0xfe, 0x01},
    {0x74, 0x01},
// AEC
    {0xfe, 0x01},
    {0x01, 0x08},
    {0x02, 0xc0},
    {0x03, 0x04},
    {0x04, 0x90},
    {0x05, 0x30},
    {0x06, 0x98},
    {0x07, 0x28},
    {0x08, 0x6c},
    {0x0a, 0xc2},
    {0x21, 0x15}, // if 0xfa=11, then 0x21=15;else if 0xfa=00,then 0x21=14
    {0xfe, 0x00},
// gamma
    {0xfe, 0x00},
    {0xc3, 0x00}, // if shutter/2 when capture, then exp_gamma_th/2
    {0xc4, 0x90},
    {0xc5, 0x98},
    {0xfe, 0x00},
    ENDMARKER,
};


static const struct camera_regval_list gc2155_qvga_regs[] = {
    {0x95,0x00},// win_size 320 * 240
    {0x96,0xf0},
    {0x97,0x01},
    {0x98,0x40},
    ENDMARKER,
};

static const struct camera_regval_list gc2155_vga_regs[] = {
    {0x95,0x01},//win_size 640 * 480
    {0x96,0xe0},
    {0x97,0x02},
    {0x98,0x80},
    ENDMARKER,
};

static const struct camera_regval_list gc2155_720P_regs[] = {
    // 720P init
    {0xfe,0x00},
    {0xb6,0x01},
    {0xfd,0x00},
    // crop window
    {0xfe,0x00},
    {0x99,0x55},
    {0x9a,0x06},
    {0x9b,0x00},
    {0x9c,0x00},
    {0x9d,0x01},
    {0x9e,0x23},
    {0x9f,0x00},
    {0xa0,0x00},
    {0xa1,0x01},
    {0xa2,0x23},
    {0x90,0x01},
    {0x91,0x00},
    {0x92,0x78},
    {0x93,0x00},
    {0x94,0x00},
    {0x95,0x02},
    {0x96,0xd0},
    {0x97,0x05},
    {0x98,0x00},
    // AWB
    {0xfe,0x00},
    {0xec,0x02},
    {0xed,0x04},
    {0xee,0x60},
    {0xef,0x90},
    {0xfe,0x01},
    {0x74,0x01},
    // AEC
    {0xfe,0x01},
    {0x01,0x08},
    {0x02,0xc0},
    {0x03,0x04},
    {0x04,0x90},
    {0x05,0x30},
    {0x06,0x98},
    {0x07,0x28},
    {0x08,0x6c},
    {0x0a,0xc2},
    {0x21,0x15},
    {0xfe,0x00},
    //banding setting 20fps fixed///
    {0xfe,0x00},
    {0x03,0x03},
    {0x04,0xe8},
    {0x05,0x01},
    {0x06,0x56},
    {0x07,0x00},
    {0x08,0x32},
    {0xfe,0x01},
    {0x25,0x00},
    {0x26,0xfa},
    {0x27,0x04},
    {0x28,0xe2}, //20fps
    {0x29,0x04},
    {0x2a,0xe2}, //16fps   5dc
    {0x2b,0x04},
    {0x2c,0xe2}, //16fps  6d6  5dc
    {0x2d,0x04},
    {0x2e,0xe2}, //8fps    bb8
    {0x3c,0x00}, //8fps
    {0xfe,0x00},
    ENDMARKER,
};

static const struct image_fmt gc2155_supported_fmts[] = {
    /* 分辨率从小到大排列 */
    IMAGE_FMT("QVGA", W_QVGA, H_QVGA, gc2155_qvga_regs),
    IMAGE_FMT("VGA", W_VGA, H_VGA, gc2155_vga_regs),
    IMAGE_FMT("720P", W_720P, H_720P, gc2155_720P_regs),
    IMAGE_FMT(0, 0, 0, 0),
};

static const struct camera_regval_list gc2155_wb_auto_regs[] = {
    ENDMARKER,
};

static const struct camera_regval_list gc2155_wb_incandescence_regs[] = {
    ENDMARKER,
};

static const struct camera_regval_list gc2155_wb_daylight_regs[] = {
    ENDMARKER,
};

static const struct camera_regval_list gc2155_wb_fluorescent_regs[] = {
    ENDMARKER,
};

static const struct camera_regval_list gc2155_wb_cloud_regs[] = {
    ENDMARKER,
};

static const struct camera_regval_list gc2155_effect_normal_regs[] = {
    ENDMARKER,
};

static const struct camera_regval_list gc2155_effect_grayscale_regs[] = {
    ENDMARKER,
};

static const struct camera_regval_list gc2155_effect_sepia_regs[] = {
    ENDMARKER,
};

static const struct camera_regval_list gc2155_effect_colorinv_regs[] = {
    ENDMARKER,
};

static const struct camera_regval_list gc2155_effect_sepiabluel_regs[] = {
    ENDMARKER,
};

#endif
