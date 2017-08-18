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

TESTUNIT_HID := test_usb_hid
TESTUNIT_CRC_ACM := test_usb_cdc_acm
TESTUNIT_SWITCH:= test_usb_switch

TESTUNIT_HID_OBJS := main_hid.o
TESTUNIT_CRC_ACM_OBJS := main_cdc_acm.o
TESTUNIT_SWITCH_OBJS := main_switch.o
LINK_LIBS := -L$(LIBS_OUTDIR) -l$(TARGET_NAME)

.PHONY : all clean

all: $(TESTUNIT_HID) $(TESTUNIT_CRC_ACM)  $(TESTUNIT_SWITCH)

%.o:%.c
	$(QUIET_CC) $(COMPILE_SRC) $< -o $@

$(TESTUNIT_HID):$(TESTUNIT_HID_OBJS)
	$(QUIET_LINK)$(LINK_OBJS) $(LINK_LIBS) -o $(OUTDIR)/$@ $^ $(LDFLAGS)

$(TESTUNIT_CRC_ACM):$(TESTUNIT_CRC_ACM_OBJS)
	$(QUIET_LINK)$(LINK_OBJS) $(LINK_LIBS) -o $(OUTDIR)/$@ $^ $(LDFLAGS)

$(TESTUNIT_SWITCH):$(TESTUNIT_SWITCH_OBJS)
	$(QUIET_LINK)$(LINK_OBJS) $(LINK_LIBS) -o $(OUTDIR)/$@ $^ $(LDFLAGS)
clean:
	@rm -rf $(TESTUNIT_HID_OBJS) $(TESTUNIT_CRC_ACM_OBJS) $(TESTUNIT_SWITCH_OBJS)
