#ifndef USB_MANAGER_H
#define USB_MANAGER_H

#include <limits.h>
#include <libqrcode_api.h>

/**
 * USB basic dependence
 */
 // #define USB_DEBUG
 /* driver module: default parser seperator */
 #define USB_DRIVER_MODULE_DEFAULT_SEPRATOR ":"
/* driver module: switch limits count*/
#define USB_SWITCH_DEVICE_RETRY_COUNT              5
/* otg switch wait time in miliseconds */
#define USB_SWITCH_DEVICE_WAITTIME_MS               100
/* gadget driver module name */
#define USB_DEV_BASE_MODULE                                 "libcomposite"
struct usb_dev_modules {
    char *dev;
    char *module;
    int32_t transfer_unit;
};

struct dynamic_array {
    uint32_t size;
    char** element;
};

 struct usb_dev {
    char name[NAME_MAX];
    uint8_t id;
    uint8_t in_use;
    int32_t fd;
    struct dynamic_array darray;
};

/**
 * USB device hid
 */
/* USB hid device name prefix */
#define USB_DEV_HID_NODE_PREFIX                         "/dev/hidg"
/* USB hid device gadget driver module name, must assigned in same order with rmmod */
#define USB_DEV_HID_DRIVER_MODULE                    "g_hid"
/* USB hid device: max packet size support */
#define USB_DEV_HID_MAX_PKT_SIZE                         8

/**
 * USB device cdc acm
 */
/* USB cdc acm device name prefix */
#define USB_DEV_CDC_ACM_NODE_PREFIX                 "/dev/ttyGS"
/* USB hid device gadget driver module name, must assigned in same order with rmmod */
#define USB_DEV_CDC_ACM_DRIVER_MODULE           "g_serial:usb_f_acm:u_serial"
/* USB cdc acm device: max write buffer size*/
#define USB_DEV_CDC_ACM_MAX_WRITE_SIZE           8192

#endif