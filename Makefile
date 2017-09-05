 #
 #  Copyright (C) 2017, Zhang YanMing <yanmin.zhang@ingenic.com, jamincheung@126.com>
 #
 #  Ingenic Linux plarform SDK project
 #
 #  This program is free software; you can redistribute it and/or modify it
 #  under  the terms of the GNU General  Public License as published by the
 #  Free Software Foundation;  either version 2 of the License, or (at your
 #  option) any later version.
 #
 #  You should have received a copy of the GNU General Public License along
 #  with this program; if not, write to the Free Software Foundation, Inc.,
 #  675 Mass Ave, Cambridge, MA 02139, USA.
 #
 #

ifneq (config.mk, $(wildcard config.mk))
$(error Could not find $(shell pwd)/config.mk !!!)
endif

include config.mk

#
# Utils
#
OBJS-y += utils/compare_string.o                                               \
          utils/assert.o                                                       \
          utils/dump_stack.o                                                   \
          utils/common.o                                                       \
          utils/file_ops.o                                                     \
          utils/yuv2bmp.o

OBJS-$(CONFIG_LIB_PNG) += utils/png_decode.o
OBJS-$(CONFIG_ALSA_AUDIO) += utils/wave_parser.o
OBJS-$(CONFIG_MINI_ZIP) += utils/minizip.o

#
# Thread
#
OBJS-y += thread/pthread_wrapper.o                                             \
          thread/thread.o

#
# Signal handler
#
OBJS-y += signal/signal_handler.o


#
# Netlink
#
OBJS-$(CONFIG_NETLINK_MANAGER) += netlink/netlink_manager.o                    \
                                  netlink/netlink_listener.o                   \
                                  netlink/netlink_handler.o                    \
                                  netlink/netlink_event.o

#
# Ring buffer
#
OBJS-$(CONFIG_RING_BUFFER) += ring_buffer/ring_buffer.o

#
# Timer
#
OBJS-$(CONFIG_TIMER_MANAGER) += timer/timer_manager.o

#
# Uart
#
OBJS-$(CONFIG_UART_MANAGER) += uart/uart_manager.o

#
# zigbee
#
OBJS-$(CONFIG_ZIGBEE_MANAGER) += zigbee/zigbee/zigbee_manager.o                \
                                 zigbee/protocol/uart_protocol.o

#
# GPIO
#
OBJS-$(CONFIG_GPIO_MANAGER) += gpio/gpio_manager.o

#
# Camera(char device)
#
OBJS-$(CONFIG_CHAR_CAMERA_MANAGER) += camera/camera_manager.o

#
# Camera(V4L2)
#
OBJS-$(CONFIG_V4L2_CAMERA_MANAGER) += camera_v4l2/cim/capture.o                \
                                      camera_v4l2/cim/jz_mem.o                 \
                                      camera_v4l2/camera_v4l2_manager.o

#
# PWM
#
OBJS-$(CONFIG_PWM_MANAGER) += pwm/pwm_manager.o

#
# Watchdog
#
OBJS-$(CONFIG_WATCHDOG_MANAGER) += watchdog/watchdog_manager.o

#
# Power
#
OBJS-$(CONFIG_POWER_MANAGER) += power/power_manager.o

#
# I2C
#
OBJS-$(CONFIG_I2C_MANAGER) += i2c/i2c_manager.o

#
# Efuse
#
OBJS-$(CONFIG_EFUSE_MANAGER) += efuse/efuse_manager.o

#
# RTC
#
OBJS-$(CONFIG_RTC_MANAGER) += rtc/rtc_manager.o

#
# SPI
#
OBJS-$(CONFIG_SPI_MANAGER) += spi/spi_manager.o

#
# 74HC595
#
OBJS-$(CONFIG_74HC595_MANAGER) += 74hc595/74hc595_manager.o

#
# Cypress
#
OBJS-$(CONFIG_CYPRESS_MANAGER) += cypress/cypress_manager.o

#
# Flash
#
OBJS-$(CONFIG_MTD_FLASH_MANAGER) += flash/flash_manager.o                      \
                                    flash/block/blocks/mtd/mtd.o               \
                                    flash/block/blocks/mtd/base.o              \
                                    flash/block/fs/fs_manager.o                \
                                    flash/block/fs/normal.o

#
# USB
#
OBJS-$(CONFIG_USB_DEVICE_MANAGER) += usb/usb_device_manager.o

#
# Security
#
OBJS-$(CONFIG_SECURITY_MANAGER) += security/security_manager.o

#
# Input
#
OBJS-$(CONFIG_INPUT_MANAGER) += input/input_manager.o                          \
                                input/input_event_callback_list.o              \
                                input/input_event_callback.o

#
# Battery
#
OBJS-$(CONFIG_BATTERY_MANAGER) += battery/battery_manager.o

#
# Alarm
#
OBJS-$(CONFIG_ALARM_MANAGER) += alarm/alarm_manager.o

#
# Vibrator
#
OBJS-$(CONFIG_VIBRATOR_MANAGER) += vibrator/vibrator_manager.o

#
# Mount
#
OBJS-$(CONFIG_MOUNT_MANAGER) += mount/mount_manager.o

#
# Framebuffer
#
OBJS-$(CONFIG_FRMAEBUFFER_MANAGER) += fb/fb_manager.o

#
# Graphics
#
OBJS-$(CONFIG_GRAPHICS_DRAWER) += graphics/gr_drawer.o

#
# Fingerprint
#
OBJS-$(CONFIG_LIB_FINGERPRINT_FPC) += fingerprint/fpc/fpc_fingerprint.o

#
# Audio
#
OBJS-$(CONFIG_ALSA_AUDIO) += audio/alsa/wave_pcm_common.o
OBJS-$(CONFIG_ALSA_AUDIO_PLAYER) += audio/alsa/wave_player.o
OBJS-$(CONFIG_ALSA_AUDIO_RECORDER) += audio/alsa/wave_recorder.o
OBJS-$(CONFIG_ALSA_AUDIO_MIXER) += audio/alsa/mixer_controller.o

OBJS := $(OBJS-y)

#
# Mini-zip
#
LIBS-$(CONFIG_LIB_MINI_ZIP) += lib/zip/minizip/ioapi.o                         \
                               lib/zip/minizip/unzip.o                         \
                               lib/zip/minizip/zip.o

#
# XML Lib
#
LIBS-$(CONFIG_LIB_MXML) += lib/mxml/mxml-2.10/mxml-attr.o                      \
                           lib/mxml/mxml-2.10/mxml-entity.o                    \
                           lib/mxml/mxml-2.10/mxml-file.o                      \
                           lib/mxml/mxml-2.10/mxml-get.o                       \
                           lib/mxml/mxml-2.10/mxml-index.o                     \
                           lib/mxml/mxml-2.10/mxml-node.o                      \
                           lib/mxml/mxml-2.10/mxml-search.o                    \
                           lib/mxml/mxml-2.10/mxml-set.o                       \
                           lib/mxml/mxml-2.10/mxml-private.o                   \
                           lib/mxml/mxml-2.10/mxml-string.o

#
# ZLib & Mini-zip Lib
#
LIBS-$(CONFIG_LIB_ZLIB) += lib/zlib/zlib-1.2.8/adler32.o                       \
                           lib/zlib/zlib-1.2.8/crc32.o                         \
                           lib/zlib/zlib-1.2.8/deflate.o                       \
                           lib/zlib/zlib-1.2.8/infback.o                       \
                           lib/zlib/zlib-1.2.8/inffast.o                       \
                           lib/zlib/zlib-1.2.8/inflate.o                       \
                           lib/zlib/zlib-1.2.8/inftrees.o                      \
                           lib/zlib/zlib-1.2.8/trees.o                         \
                           lib/zlib/zlib-1.2.8/zutil.o                         \
                           lib/zlib/zlib-1.2.8/compress.o                      \
                           lib/zlib/zlib-1.2.8/uncompr.o                       \
                           lib/zlib/zlib-1.2.8/gzclose.o                       \
                           lib/zlib/zlib-1.2.8/gzlib.o                         \
                           lib/zlib/zlib-1.2.8/gzread.o                        \
                           lib/zlib/zlib-1.2.8/gzwrite.o

#
# PNG Lib
#
LIBS-$(CONFIG_LIB_PNG) += lib/png/libpng-1.6.26/png.o                          \
                          lib/png/libpng-1.6.26/pngerror.o                     \
                          lib/png/libpng-1.6.26/pngget.o                       \
                          lib/png/libpng-1.6.26/pngmem.o                       \
                          lib/png/libpng-1.6.26/pngpread.o                     \
                          lib/png/libpng-1.6.26/pngread.o                      \
                          lib/png/libpng-1.6.26/pngrio.o                       \
                          lib/png/libpng-1.6.26/pngrtran.o                     \
                          lib/png/libpng-1.6.26/pngrutil.o                     \
                          lib/png/libpng-1.6.26/pngset.o                       \
                          lib/png/libpng-1.6.26/pngtrans.o                     \
                          lib/png/libpng-1.6.26/pngwio.o                       \
                          lib/png/libpng-1.6.26/pngwrite.o                     \
                          lib/png/libpng-1.6.26/pngwtran.o                     \
                          lib/png/libpng-1.6.26/pngwutil.o

#
# MD5 Lib (Deprecated, you should use openssl - include/lib/openssl)
#
LIBS-n += lib/md5/libmd5.o

#
# SHA Lib (Deprecated, you should use openssl - include/lib/openssl)
#
LIBS-n += lib/sha/libsha1.o

#
# CRC Lib
#
LIBS-$(CONFIG_LIB_CRC) += lib/crc/libcrc.o

#
# Base64 Lib
#
LIBS-$(CONFIG_LIB_BASE64) += lib/b64/libb64-1.2/src/cencode.o                  \
                             lib/b64/libb64-1.2/src/cdecode.o

#
# Uart Lib
#
LIBS-$(CONFIG_LIB_SERIAL) += lib/uart/libserialport/linux_termios.o            \
                             lib/uart/libserialport/linux.o                    \
                             lib/uart/libserialport/serialport.o

#
# I2C Lib
#
LIBS-$(CONFIG_LIB_I2C) += lib/i2c/libsmbus.o

#
# MTD Lib
#
LIBS-$(CONFIG_LIB_MTD) += lib/mtd/libmtd_legacy.o                              \
                          lib/mtd/libmtd.o

#
# GPIO Lib
#
LIBS-$(CONFIG_LIB_GPIO) += lib/gpio/libgpio.o

LIBS := $(LIBS-y)


all: $(TARGET) examples libporter

.PHONY : all clean backup examples examples_clean

#
# Targets
#
$(TARGET): $(OBJS) $(LIBS)
	$(QUIET_LINK)$(LINK_OBJS) $(LDFLAGS) $(LDSHFLAGS) $(OBJS) $(LIBS) -o $(OUTDIR)/$@
	@$(STRIP) $(OUTDIR)/$@
	@echo -e '\n  SDK: $(shell basename $(OUTDIR))/$@ is ready\n'

#
# Libporter
#
libporter:
	@$(TOPDIR)/libporter.sh $(LDFLAGS)

#
# Examples
#
examples: $(TARGET)
	@$(MAKE) -sC examples all
	@echo -e '\n  $@: $(shell basename $(EXAMPLES_OUTDIR))/test_xxx is ready\n'

examples_clean:
	@$(MAKE) -sC examples clean

clean: examples_clean
	@rm -rf $(LIBS) $(OBJS)
	@rm -rf $(OUTDIR)

distclean: clean

backup: distclean
	F=`basename $(TOPDIR)` ; cd .. ; \
	tar --force-local --exclude=.git -Jcvf `date "+$$F-%Y-%m-%d-%T.tar.xz"` $$F
