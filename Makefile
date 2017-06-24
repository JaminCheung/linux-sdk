 #
 #  Copyright (C) 2016, Zhang YanMing <jamincheung@126.com>
 #
 #  Ingenic QRcode SDK Project
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
# Netlink
#
OBJS-y += netlink/netlink_manager.o                                            \
          netlink/netlink_listener.o                                           \
          netlink/netlink_handler.o                                            \
          netlink/netlink_event.o

#
# Utils
#
OBJS-y += utils/signal_handler.o                                               \
          utils/compare_string.o                                               \
          utils/assert.o                                                       \
          utils/common.o                                                       \
          utils/file_ops.o                                                     \
          utils/thread_pool/thread_pool.o

#
# Timer
#
OBJS-y += timer/timer_manager.o

#
# Uart
#
OBJS-y += uart/uart_manager.o

#
# GPIO
#
OBJS-y += gpio/gpio_manager.o

#
# Camera
#
OBJS-y += camera/camera_manager.o

#
# PWM
#
OBJS-y += pwm/pwm_manager.o

#
# Watchdog
#
OBJS-y += watchdog/watchdog_manager.o

#
# Power
#
OBJS-y += power/power_manager.o

#
# I2C
#
OBJS-y += i2c/i2c_manager.o

#
# Efuse
#
OBJS-y += efuse/efuse_manager.o

#
# RTC
#
OBJS-y += rtc/rtc_manager.o

#
# SPI
#
OBJS-y += spi/spi_manager.o

#
# 74HC595
#
OBJS-y += 74hc595/74hc595_manager.o

#
# Flash
#
OBJS-y += flash/flash_manager.o                                                \
          flash/block/blocks/mtd/mtd.o                                         \
          flash/block/blocks/mtd/base.o                                        \
          flash/block/fs/fs_manager.o                                          \
          flash/block/fs/normal.o
#
# USB
#
OBJS-y += usb/usb_device_manager.o

#
# Security
#
OBJS-y += security/security_manager.o

#
# Input
#
OBJS-y += input/input_manager.o

#
# Battery
#
OBJS-y += battery/battery_manager.o

#
# Alarm
#
OBJS-y += alarm/alarm_manager.o

#
# Vibrator
#
OBJS-y += vibrator/vibrator_manager.o

#
# Mount
#
OBJS-y += mount/mount_manager.o

#
# Fingerprint
#
OBJS-y += fingerprint/fingerprint_manager.o                                    \
          fingerprint/fingerprint.o                                            \
          fingerprint/fingerprint_list.o                                       \
          fingerprint/hardware/fingerprint_hal.o

OBJS := $(OBJS-y)

#
# XML Lib
#
LIBS-y += lib/mxml/mxml-2.10/mxml-attr.o                                       \
          lib/mxml/mxml-2.10/mxml-entity.o                                     \
          lib/mxml/mxml-2.10/mxml-file.o                                       \
          lib/mxml/mxml-2.10/mxml-get.o                                        \
          lib/mxml/mxml-2.10/mxml-index.o                                      \
          lib/mxml/mxml-2.10/mxml-node.o                                       \
          lib/mxml/mxml-2.10/mxml-search.o                                     \
          lib/mxml/mxml-2.10/mxml-set.o                                        \
          lib/mxml/mxml-2.10/mxml-private.o                                    \
          lib/mxml/mxml-2.10/mxml-string.o

#
# ZLib & Mini-zip Lib
#
LIBS-y += lib/zlib/zlib-1.2.8/adler32.o                                        \
          lib/zlib/zlib-1.2.8/crc32.o                                          \
          lib/zlib/zlib-1.2.8/deflate.o                                        \
          lib/zlib/zlib-1.2.8/infback.o                                        \
          lib/zlib/zlib-1.2.8/inffast.o                                        \
          lib/zlib/zlib-1.2.8/inflate.o                                        \
          lib/zlib/zlib-1.2.8/inftrees.o                                       \
          lib/zlib/zlib-1.2.8/trees.o                                          \
          lib/zlib/zlib-1.2.8/zutil.o                                          \
          lib/zlib/zlib-1.2.8/compress.o                                       \
          lib/zlib/zlib-1.2.8/uncompr.o                                        \
          lib/zlib/zlib-1.2.8/gzclose.o                                        \
          lib/zlib/zlib-1.2.8/gzlib.o                                          \
          lib/zlib/zlib-1.2.8/gzread.o                                         \
          lib/zlib/zlib-1.2.8/gzwrite.o                                        \

#
# MD5 Lib
#
LIBS-y += lib/md5/libmd5.o

#
# CRC Lib
#
LIBS-y += lib/crc/libcrc.o

#
# Base64 Lib
#
LIBS-y += lib/b64/libb64-1.2/src/cencode.o                                     \
          lib/b64/libb64-1.2/src/cdecode.o

#
# Uart Lib
#
LIBS-y += lib/uart/libserialport/linux_termios.o                               \
          lib/uart/libserialport/linux.o                                       \
          lib/uart/libserialport/serialport.o

#
# I2c Lib
#
LIBS-y += lib/i2c/libsmbus.o

#
# MTD Lib
#
LIBS-y += lib/mtd/libmtd_legacy.o                                              \
          lib/mtd/libmtd.o

#
# lib gpio serving for gpio
#
LIBS-y += lib/gpio/libgpio.o

LIBS := $(LIBS-y)


all: $(TARGET) testunit

.PHONY : all testunit testunit_clean clean backup

#
# Targets
#
$(TARGET): $(OBJS) $(LIBS)
	$(QUIET_LINK)$(LINK_OBJS) $(LDFLAGS) $(LDSHFLAGS) $(OBJS) $(LIBS) -o $(OUTDIR)/$@
	@$(STRIP) $(OUTDIR)/$@
	@echo -e '\n  sdk: $(shell basename $(OUTDIR))/$@ is ready\n'

#
# Test unit
#
testunit:
	make -C uart/testunit all
	make -C gpio/testunit all
	make -C camera/testunit all
	make -C pwm/testunit all
	make -C timer/testunit all
	make -C watchdog/testunit all
	make -C power/testunit all
	make -C i2c/testunit all
	make -C flash/testunit all
	make -C efuse/testunit all
	make -C rtc/testunit all
	make -C spi/testunit all
	make -C usb/testunit all
	make -C security/testunit all
	make -C battery/testunit all
	make -C mount/testunit all
	make -C 74hc595/testunit all
	make -C input/testunit all
	make -C vibrator/testunit all
	make -C fingerprint/testunit all

testunit_clean:
	make -C uart/testunit clean
	make -C gpio/testunit clean
	make -C camera/testunit clean
	make -C pwm/testunit clean
	make -C timer/testunit clean
	make -C watchdog/testunit clean
	make -C power/testunit clean
	make -C i2c/testunit clean
	make -C flash/testunit clean
	make -C efuse/testunit clean
	make -C rtc/testunit clean
	make -C spi/testunit clean
	make -C usb/testunit clean
	make -C security/testunit clean
	make -C battery/testunit clean
	make -C mount/testunit clean
	make -C 74hc595/testunit clean
	make -C input/testunit clean
	make -C vibrator/testunit clean
	make -C fingerprint/testunit clean

clean: testunit_clean
	@rm -rf $(LIBS) $(OBJS)
	@rm -rf $(OUTDIR)

distclean: clean
