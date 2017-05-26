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
OBJ-y += netlink/netlink_manager.o                                            \
          netlink/netlink_listener.o                                           \
          netlink/netlink_handler.o                                            \
          netlink/netlink_event.o

#
# Utils
#
OBJ-y += utils/signal_handler.o                                                \
         utils/compare_string.o                                                \
         utils/assert.o                                                        \
         utils/common.o                                                        \
         utils/thread_pool/thread_pool.o

#
# Timer
#
OBJ-y += timer/timer_manager.o

#
# Uart
#
OBJ-y += uart/uart_manager.o

#
# GPIO
#
OBJ-y += gpio/gpio_manager.o

#
# Camera
#
OBJ-y += camera/camera_manager.o

#
# PWM
#
OBJ-y += pwm/pwm_manager.o

#
# Watchdog
#
OBJ-y += watchdog/watchdog_manager.o

#
# Power
#
OBJ-y += power/power_manager.o

#
# I2C
#
OBJ-y += i2c/i2c_manager.o

#
# Efuse
#
OBJ-y += efuse/efuse_manager.o

#
# RTC
#
OBJ-y += rtc/rtc_manager.o

#
# SPI
#
OBJ-y += spi/spi_manager.o

#
# Flash
#
OBJ-y +=  flash/flash_manager.o                                                \
          flash/block/blocks/mtd/mtd.o                                         \
          flash/block/blocks/mtd/base.o                                        \
          flash/block/fs/fs_manager.o                                          \
          flash/block/fs/normal.o
#
# USB
#
OBJ-y += usb/usb_device_manager.o

#
# Security
#
OBJ-y += security/security_manager.o

#
# Input
#
OBJ-y += input/input_manager.o

#
# Battery
#
OBJ-y += battery/battery_manager.o

OBJS := $(OBJ-y)

#
# Uart Lib
#
LIBS-OBJ-y += lib/uart/libserialport/linux_termios.o    \
          lib/uart/libserialport/linux.o                \
          lib/uart/libserialport/serialport.o           \

#
# I2c Lib
#
LIBS-OBJ-y += lib/i2c/libsmbus.o

#
# MTD Lib
#
LIBS-OBJ-y += lib/mtd/libmtd_legacy.o                   \
              lib/mtd/libmtd.o

#
# lib gpio serving for gpio
#
LIBS-OBJ-y += lib/gpio/libgpio.o

LIBS-OBJS := $(LIBS-OBJ-y)


all: $(TARGET) testunit

.PHONY : all testunit testunit_clean clean backup

#
# Targets
#
$(TARGET): $(OBJS) $(LIBS-OBJS)
	$(QUIET_LINK)$(LINK_OBJS) $(LDFLAGS) $(LDSHFLAGS) $(OBJS) $(LIBS-OBJS) -o $(OUTDIR)/$@
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

clean: testunit_clean
	@rm -rf $(LIBS-OBJS) $(OBJS)
	@rm -rf $(OUTDIR)

distclean: clean
