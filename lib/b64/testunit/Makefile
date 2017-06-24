TOPDIR ?= ../../..
#CROSS_COMILE ?=

include ../../../config.mk

TESTUNIT := test_libb64
TESTUNIT_OBJS := main.o

.PHONY : all clean

all: $(TESTUNIT)

$(TESTUNIT): $(TESTUNIT_OBJS)
	$(QUIET_LINK)$(LINK_OBJS) -o $(OUTDIR)/$@ $(TESTUNIT_OBJS) $(LDFLAGS) $(LDLIBS)

clean:
	rm -rf $(TESTUNIT_OBJS)
