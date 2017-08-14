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
          utils/png_decode.o                                                   \
          utils/wave_parser.o                                                  \
          utils/runnable/pthread_wrapper.o                                     \
          utils/runnable/default_runnable.o                                    \
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
# zigbee
#
OBJS-y += zigbee/protocol/uart_protocol.o
OBJS-y += zigbee/zigbee/zigbee_manager.o

#
# GPIO
#
OBJS-y += gpio/gpio_manager.o

#
# Camera
#
OBJS-y += camera/camera_manager.o

#
# Cimutils
#
OBJS-y += camera_v4l2/cim/capture.o                                            \
          camera_v4l2/cim/yuv2bmp.o                                            \
          camera_v4l2/cim/jz_mem.o                                             \
          camera_v4l2/camera_v4l2_manager.o

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
# Cypress
#
OBJS-y += cypress/cypress_manager.o

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
OBJS-y += input/input_manager.o                                                \
          input/input_event_callback_list.o                                    \
          input/input_event_callback.o

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
# Framebuffer
#
OBJS-y += fb/fb_manager.o

#
# Graphics
#
OBJS-y += graphics/gr_drawer.o

#
# Fingerprint
#
OBJS-n += fingerprint/fingerprint_manager.o                                    \
          fingerprint/fingerprint.o                                            \
          fingerprint/fingerprint_list.o                                       \
          fingerprint/fingerprints_userstate.o                                 \
          fingerprint/fingerprint_utils.o                                      \
          fingerprint/authentication_result.o                                  \
          fingerprint/client_monitor.o                                         \
          fingerprint/enroll_client.o                                          \
          fingerprint/removal_client.o                                         \
          fingerprint/authentication_client.o                                  \
          fingerprint/enumerate_client.o                                       \
          fingerprint/hardware/android/hardware.o                              \
          fingerprint/hardware/fingerprint_device_proxy.o

#
# Fingerprint2
#
OBJS-n += fingerprint2/fingerprint_manager.o

#
# Audio
#
OBJS-y += audio/alsa/wave_pcm_common.o                                         \
          audio/alsa/wave_player.o                                             \
          audio/alsa/wave_recorder.o

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
# PNG Lib
#
LIBS-y += lib/png/libpng-1.6.26/png.o                                          \
          lib/png/libpng-1.6.26/pngerror.o                                     \
          lib/png/libpng-1.6.26/pngget.o                                       \
          lib/png/libpng-1.6.26/pngmem.o                                       \
          lib/png/libpng-1.6.26/pngpread.o                                     \
          lib/png/libpng-1.6.26/pngread.o                                      \
          lib/png/libpng-1.6.26/pngrio.o                                       \
          lib/png/libpng-1.6.26/pngrtran.o                                     \
          lib/png/libpng-1.6.26/pngrutil.o                                     \
          lib/png/libpng-1.6.26/pngset.o                                       \
          lib/png/libpng-1.6.26/pngtrans.o                                     \
          lib/png/libpng-1.6.26/pngwio.o                                       \
          lib/png/libpng-1.6.26/pngwrite.o                                     \
          lib/png/libpng-1.6.26/pngwtran.o                                     \
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
# I2C Lib
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
	@echo -e '\n  SDK: $(shell basename $(OUTDIR))/$@ is ready\n'

#
# Test unit
#
testunit:
	$(QUITE_TEST_BUILD)test_camerav4l2;make -sC camera_v4l2/testunit all
	$(QUITE_TEST_BUILD)test_zigbee;make -sC zigbee/testunit all
	$(QUITE_TEST_BUILD)test_uart;make -sC uart/testunit all
	$(QUITE_TEST_BUILD)test_gpio;make -sC gpio/testunit all
	$(QUITE_TEST_BUILD)test_camera;make -sC camera/testunit all
	$(QUITE_TEST_BUILD)test_pwm;make -sC pwm/testunit all
	$(QUITE_TEST_BUILD)test_timer;make -sC timer/testunit all
	$(QUITE_TEST_BUILD)test_watchdog;make -sC watchdog/testunit all
	$(QUITE_TEST_BUILD)test_power;make -sC power/testunit all
	$(QUITE_TEST_BUILD)test_i2c;make -sC i2c/testunit all
	$(QUITE_TEST_BUILD)test_flash;make -sC flash/testunit all
	$(QUITE_TEST_BUILD)test_efuse;make -sC efuse/testunit all
	$(QUITE_TEST_BUILD)test_rtc;make -sC rtc/testunit all
	$(QUITE_TEST_BUILD)test_spi;make -sC spi/testunit all
	$(QUITE_TEST_BUILD)test_usb;make -sC usb/testunit all
	$(QUITE_TEST_BUILD)test_security;make -sC security/testunit all
	$(QUITE_TEST_BUILD)test_battery;make -sC battery/testunit all
	$(QUITE_TEST_BUILD)test_mount;make -sC mount/testunit all
	$(QUITE_TEST_BUILD)test_74hc595;make -sC 74hc595/testunit all
	$(QUITE_TEST_BUILD)test_cypress;make -sC cypress/testunit all
	$(QUITE_TEST_BUILD)test_input;make -sC input/testunit all
	$(QUITE_TEST_BUILD)test_vibrator;make -sC vibrator/testunit all
	$(QUITE_TEST_BUILD)test_fingerprint2;make -sC fingerprint2/testunit all
	$(QUITE_TEST_BUILD)test_default_runnable;make -sC utils/runnable/testunit all
	$(QUITE_TEST_BUILD)test_fb;make -sC fb/testunit all
	$(QUITE_TEST_BUILD)test_graphics;make -sC graphics/testunit all
	$(QUITE_TEST_BUILD)test_wave_play;make -sC audio/alsa/testunit all
	$(QUITE_TEST_BUILD)test_wave_recod;make -sC audio/alsa/testunit all

	@echo -e '\n  $@: $(shell basename $(OUTDIR))/test_xxx is ready\n'

testunit_clean:
	$(QUITE_TEST_CLEAN)test_camerav4l2;make -sC camera_v4l2/testunit clean
	$(QUITE_TEST_CLEAN)test_zigbee;make -sC zigbee/testunit clean
	$(QUITE_TEST_CLEAN)test_uart;make -sC uart/testunit clean
	$(QUITE_TEST_CLEAN)test_gpio;make -sC gpio/testunit clean
	$(QUITE_TEST_CLEAN)test_camera;make -sC camera/testunit clean
	$(QUITE_TEST_CLEAN)test_pwm;make -sC pwm/testunit clean
	$(QUITE_TEST_CLEAN)test_timer;make -sC timer/testunit clean
	$(QUITE_TEST_CLEAN)test_watchdog;make -sC watchdog/testunit clean
	$(QUITE_TEST_CLEAN)test_power;make -sC power/testunit clean
	$(QUITE_TEST_CLEAN)test_i2c;make -sC i2c/testunit clean
	$(QUITE_TEST_CLEAN)test_flash;make -sC flash/testunit clean
	$(QUITE_TEST_CLEAN)test_efuse;make -sC efuse/testunit clean
	$(QUITE_TEST_CLEAN)test_rtc;make -sC rtc/testunit clean
	$(QUITE_TEST_CLEAN)test_spi;make -sC spi/testunit clean
	$(QUITE_TEST_CLEAN)test_usb;make -sC usb/testunit clean
	$(QUITE_TEST_CLEAN)test_security;make -sC security/testunit clean
	$(QUITE_TEST_CLEAN)test_battery;make -sC battery/testunit clean
	$(QUITE_TEST_CLEAN)test_mount;make -sC mount/testunit clean
	$(QUITE_TEST_CLEAN)test_74hc595;make -sC 74hc595/testunit clean
	$(QUITE_TEST_CLEAN)test_cypress;make -sC cypress/testunit clean
	$(QUITE_TEST_CLEAN)test_input;make -sC input/testunit clean
	$(QUITE_TEST_CLEAN)test_vibrator;make -sC vibrator/testunit clean
	$(QUITE_TEST_CLEAN)test_fingerprint2;make -sC fingerprint2/testunit clean
	$(QUITE_TEST_CLEAN)test_default_runnable;make -sC utils/runnable/testunit clean
	$(QUITE_TEST_CLEAN)test_graphics;make -sC graphics/testunit clean
	$(QUITE_TEST_BUILD)test_wave_play;make -sC audio/alsa/testunit clean
	$(QUITE_TEST_BUILD)test_wave_recod;make -sC audio/alsa/testunit clean
clean: testunit_clean
	@rm -rf $(LIBS) $(OBJS)
	@rm -rf $(OUTDIR)

distclean: clean
