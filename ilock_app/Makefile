#
# Copyright (C) 2017 Ingenic Semiconductor
#
# YangHuanHuan <huanhuan.yang@ingenic.com, peterlihong@qq.com>
#
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; either version 2 of the License, or (at your
# option) any later version.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 675 Mass Ave, Cambridge, MA 02139, USA.
#
#

#
# Top directory
#

CURRENT_DIR = $(shell pwd)
TOPDIR ?= ..


#
# Cross cimpile
#
include ../config.mk

LINK_LIBS := -L$(LIBS_OUTDIR) -l$(TARGET_NAME)

INCLUDES += -I$(CURRENT_DIR)/include

#
# Target
#
BUILD_TARGET = ilock_app

#
# Src
#
TARGET_OBJS += input_mode/fingerprint_input.o
TARGET_OBJS += input_mode/keyboard_input.o
TARGET_OBJS += input_mode/ilock_input_manager.o
TARGET_OBJS += power_manager/power_manager.o
TARGET_OBJS += door/door_manager.o
TARGET_OBJS += main/main.o


.PHONY : all clean Tips

all: $(BUILD_TARGET) Tips

Tips: $(BUILD_TARGET)
	@echo -e '\n Target: "$(CURRENT_DIR)/../out/$(BUILD_TARGET)" is ready\n'

%.o:%.c
	$(CC) $(CFLAGS) $(INCLUDES)  -c $< -o $@

$(BUILD_TARGET):$(TARGET_OBJS)
	$(QUIET_LINK)$(LINK_OBJS) $(LINK_LIBS) -o $(OUTDIR)/$@ $(TARGET_OBJS) $(LDFLAGS)

clean:
	@rm -f $(TARGET_OBJS)
	@rm -f $(BUILD_TARGET)
	@echo -e '\n#### Clean up completed successfully ####'
