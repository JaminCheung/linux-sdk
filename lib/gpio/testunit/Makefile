TOPDIR ?= ../../..
#CROSS_COMPILE ?=

include ../../../config.mk

TESTUNIT := test_libgpio
TESTUNIT_OBJS := main.o                                                        \
          $(TOPDIR)/lib/gpio/libgpio.o

.PHONY : all clean

all: $(TESTUNIT)

$(TESTUNIT): $(TESTUNIT_OBJS)
	$(QUIET_LINK)$(LINK_OBJS) -o $(OUTDIR)/$@ $(TESTUNIT_OBJS) $(LDFLAGS)

clean:
	rm -rf $(TESTUNIT_OBJS)
