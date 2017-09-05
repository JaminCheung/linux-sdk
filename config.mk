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

SHELL := $(shell if [ -x "$$BASH" ]; then echo $$BASH; \
        else if [ -x /bin/bash ]; then echo /bin/bash; \
        else echo sh; fi; fi)

#
# Top directory
#
TOPDIR ?= $(shell pwd)

ifneq ($(TOPDIR)/build.mk, $(wildcard $(TOPDIR)/build.mk))
$(error Could not find $(TOPDIR)/build.mk !!!)
endif

include $(TOPDIR)/build.mk

TARGET_NAME := ingenic
TARGET := lib/lib$(TARGET_NAME).so

#
# Out & Tools directory
#
OUTDIR := $(TOPDIR)/out
$(shell [ -d $(OUTDIR) ] || mkdir -p $(OUTDIR))
$(if $(OUTDIR),,$(error output directory "$(OUTDIR)" does not exist))

LIBS_OUTDIR := $(OUTDIR)/lib
$(shell [ -d $(LIBS_OUTDIR) ] || mkdir -p $(LIBS_OUTDIR))
$(if $(LIBS_OUTDIR),,$(error output directory "$(LIBS_OUTDIR)" does not exist))

EXAMPLES_OUTDIR := $(OUTDIR)/bin/examples
$(shell [ -d $(EXAMPLES_OUTDIR) ] || mkdir -p $(EXAMPLES_OUTDIR))
$(if $(EXAMPLES_OUTDIR),,$(error output directory "$(EXAMPLES_OUTDIR)" does not exist))

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

CFLAGS := -O2 -g -std=gnu11 $(INCLUDES) -fPIC -D_GNU_SOURCE -mhard-float       \
          -funwind-tables
CHECKFLAGS := -Wall -Wuninitialized -Wundef
LDSHFLAGS := -shared -Wl,-Bsymbolic

LDFLAGS := -rdynamic

#
# Library link - Static
#
LIBS_STATIC_FLAGS += -Wl,-Bstatic

ifeq ($(CONFIG_LIB_FINGERPRINT_GD), y)
LIBS_STATIC_FLAGS += -L$(TOPDIR)/lib/fingerprint -lgoodix_fingerprint
endif
#
# Put your static library here
#



#
# Library link - Dynamic
#
LIBS_DYNAMIC_FLAGS += -Wl,-Bdynamic -pthread -lm -lrt -ldl -lstdc++

ifeq ($(CONFIG_LIB_ALSA), y)
LIBS_DYNAMIC_FLAGS += -L$(TOPDIR)/lib/alsa -lasound
endif

ifeq ($(CONFIG_LIB_OPENSSL), y)
LIBS_DYNAMIC_FLAGS += -L$(TOPDIR)/lib/openssl -lcrypto -lssl
endif

ifeq ($(CONFIG_LIB_FINGERPRINT_MA), y)
LIBS_DYNAMIC_FLAGS += -L$(TOPDIR)/lib/fingerprint -lfprint-mips
endif

ifeq ($(CONFIG_LIB_FACE_DETECT), y)
LIBS_DYNAMIC_FLAGS += -L$(TOPDIR)/lib/face -lNmIrFaceSdk
endif

ifeq ($(CONFIG_LIB_FINGERPRINT_FPC), y)
LIBS_DYNAMIC_FLAGS += -L$(TOPDIR)/lib/fingerprint -lfps_360_linux
endif
#
# Put your dynamic library here
#

override LDFLAGS := $(LDFLAGS) $(LIBS_STATIC_FLAGS) $(LIBS_DYNAMIC_FLAGS)

ifeq ($(CONFIG_LOCAL_DEBUG), y)
CFLAGS += -DLOCAL_DEBUG -DDEBUG
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

QUITE_TEST_BUILD = @echo -e "  BUILD\t"
QUITE_TEST_CLEAN = @echo -e "  CLEAN\t"
endif

%.o:%.c
	$(QUIET_CC) $(COMPILE_SRC) $< -o $@
