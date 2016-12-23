TOPDIR ?= ../../..
#CROSS_COMPILE ?=

include ../../../config.mk

TESTUNIT := test_libserialport
TESTUNIT_OBJS := main.o                                                        \
          $(TOPDIR)/lib/uart/libserialport/serialport.o                      \
          $(TOPDIR)/lib/uart/libserialport/linux_termios.o                   \
          $(TOPDIR)/lib/uart/libserialport/linux.o

.PHONY : all clean

all: $(TESTUNIT)

$(TESTUNIT): $(TESTUNIT_OBJS)
	$(QUIET_LINK)$(LINK_OBJS) -o $(OUTDIR)/$@ $(TESTUNIT_OBJS) $(LDFLAGS)

clean:
	@rm -rf $(TESTUNIT_OBJS)
