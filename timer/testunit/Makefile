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
TOPDIR ?= ../..

include $(TOPDIR)/config.mk

TESTUNIT_SIMPLE := test_timer_simple
TESTUNIT_HARD := test_timer_hard

TESTUNIT_OBJS_SIMPLE := main_01simple.o
TESTUNIT_OBJS_HARD := main_02hard.o

LINK_LIBS := -L$(LIBS_OUTDIR) -l$(TARGET_NAME)

.PHONY : all clean

all: $(TESTUNIT_SIMPLE) $(TESTUNIT_HARD)

%.o:%.c
	$(QUIET_CC) $(COMPILE_SRC) $< -o $@

$(TESTUNIT_SIMPLE):$(TESTUNIT_OBJS_SIMPLE)
	$(QUIET_LINK)$(LINK_OBJS) $(LINK_LIBS) -o $(OUTDIR)/$@ $(TESTUNIT_OBJS_SIMPLE) $(LDFLAGS)

$(TESTUNIT_HARD):$(TESTUNIT_OBJS_HARD)
	$(QUIET_LINK)$(LINK_OBJS) $(LINK_LIBS) -o $(OUTDIR)/$@ $(TESTUNIT_OBJS_HARD) $(LDFLAGS)

clean:
	@rm -rf $(TESTUNIT_OBJS_SIMPLE) $(TESTUNIT_OBJS_HARD)
