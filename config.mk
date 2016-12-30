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

SHELL := $(shell if [ -x "$$BASH" ]; then echo $$BASH; \
        else if [ -x /bin/bash ]; then echo /bin/bash; \
        else echo sh; fi; fi)

TARGET_NAME := qrcode
TARGET := lib$(TARGET_NAME).so

#
# Top directory
#
TOPDIR ?= $(shell pwd)

#
# Out & Tools directory
#
OUTDIR := $(TOPDIR)/out
$(shell [ -d $(OUTDIR) ] || mkdir -p $(OUTDIR))
OUTDIR := $(shell cd $(OUTDIR) && /bin/pwd)
$(if $(OUTDIR),,$(error output directory "$(OUTDIR)" does not exist))

#
# Cross compiler
#
CROSS_COMPILE ?= mips-linux-gnu-
CC := $(CROSS_COMPILE)gcc
AR := $(CROSS_COMPILE)ar
STRIP := $(CROSS_COMPILE)strip

#
# Compiler & Linker options
#
ARFLAGS := rcv
INCLUDES := -I$(TOPDIR)/include                                                \
            -I$(TOPDIR)/include/lib                                            \
            -I$(TOPDIR)/include/lib/zlib                                       \
            -I$(TOPDIR)/include/lib/zip/minizip

CFLAGS := -std=gnu11 $(INCLUDES) -fPIC
CHECKFLAGS := -Wall -Wuninitialized -Wundef
LDFLAGS := -lpthread -lrt -Wl,-rpath,/usr/lib
LDSHFLAGS := -shared -Wl,-Bsymbolic

ifndef DEBUG
CFLAGS += -Os -mhard-float
else
CFLAGS += -g -DLOCAL_DEBUG -DDEBUG
endif

override CFLAGS := $(CHECKFLAGS) $(CFLAGS)

#
# Quiet compile
#
COMPILE_SRC := $(CC) $(CFLAGS) -c
LINK_OBJS   := $(CC) $(CFLAGS)

ifndef V
QUIET_AR        = @echo -e "  AR\t$@";
QUIET_CC        = @echo -e "  CC\t$@";
QUIET_LINK      = @echo -e "  LINK\t$@";
endif

%.o:%.c
	$(QUIET_CC) $(COMPILE_SRC) $< -o $@
