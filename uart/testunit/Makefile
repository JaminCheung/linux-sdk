TOPDIR ?= ../..

include ../../config.mk

TESTUNIT := test_uart
TESTUNIT_OBJS := main.o
DEPEND_SDK_LIB := $(TOPDIR)/out/$(TARGET)

.PHONY : all clean

all: $(TESTUNIT)

%.o:%.c
	$(QUIET_CC) $(COMPILE_SRC) $< -o $@

$(TESTUNIT):
	$(QUIET_LINK)$(LINK_OBJS) -o $(OUTDIR)/$@ $(DEPEND_SDK_LIB) $(TESTUNIT_OBJS) $(LDFLAGS) $(LDLIBS)

clean:
	rm -rf $(TESTUNIT_OBJS)