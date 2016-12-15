 #
 #  Copyright (C) 2016, Zhang YanMing <jamincheung@126.com>
 #
 #  Linux recovery updater
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
SRC-y += utils/signal_handler.c                                 \
          utils/compare_string.c                                       \
          utils/assert.c                                                      \

#
# Uart
#
SRC-y += uart/uart_manager.c

#
# Camera
#
SRC-y += camera/camera_manager.c

SRCS := $(SRC-y)

#
# lib serialport serving for Uart
#
LIBS-SRC-y += lib/serial/libserialport/linux_termios.c    \
          lib/serial/libserialport/linux.c                                  \
          lib/serial/libserialport/serialport.c

LIBS-SRCS := $(LIBS-SRC-y)


#
# Targets
#
all: $(TARGET) testunit

.PHONY : all testunit testunit_clean clean backup

#
# Test unit
#
testunit:
	make -C lib/serial/testunit all
	make -C uart/testunit all
	make -C camera/testunit all

testunit_clean:
	make -C lib/serial/testunit clean
	make -C uart/testunit clean
	make -C camera/testunit clean

$(TARGET): $(OBJS) $(LIBS)
	@echo -e '\n  sdk: $(SRCS) $(LIBS-SRCS) -o $(OUTDIR)/$@\n'
	$(QUIET_CC_SHARED_LIB)$(COMPILE_SRC_TO_SHARED_LIB) $(SRCS) $(LIBS-SRCS) -o $(OUTDIR)/$@
	#@$(STRIP) $(OUTDIR)/$@
	@echo -e '\n  sdk: $(shell basename $(OUTDIR))/$@ is ready\n'

clean:
	rm -rf $(OUTDIR)

distclean: clean
