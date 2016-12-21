TOPDIR ?= ../..

include ../../config.mk

TESTUNIT := test_flash
TESTUNIT_OBJS := main.o
DEPEND_SDK_LIB := $(TOPDIR)/out/$(TARGET)

.PHONY : all clean

all: $(TESTUNIT)

%.o:%.c
	$(QUIET_CC) $(COMPILE_SRC) $< -o $@

$(TESTUNIT):$(TESTUNIT_OBJS)
	$(QUIET_LINK)$(LINK_OBJS) $(LDFLAGS) -o $(OUTDIR)/$@  $(TESTUNIT_OBJS)

clean:
	rm -rf $(TESTUNIT_OBJS)
