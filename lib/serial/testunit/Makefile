TOPDIR ?= ../../..
#CROSS_COMPILE ?=

include ../../../config.mk

TESTUNIT := test_libserialport
TESTUNIT_OBJS := main.o                                                        \
          $(TOPDIR)/lib/serial/libserialport/serialport.o                      \
          $(TOPDIR)/lib/serial/libserialport/linux_termios.o                   \
          $(TOPDIR)/lib/serial/libserialport/linux.o

.PHONY : all clean

all: $(TESTUNIT)

$(TESTUNIT): $(TESTUNIT_OBJS)
	$(QUIET_LINK)$(LINK_OBJS) -o $(OUTDIR)/$@ $(TESTUNIT_OBJS) $(LDFLAGS) $(LDLIBS)

clean:
	rm -rf $(TESTUNIT_OBJS)
