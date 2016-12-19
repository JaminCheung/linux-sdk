TOPDIR ?= ../../..

include $(TOPDIR)/config.mk

TESTUNIT := test_threadpool
TESTUNIT_OBJS := main.o

.PHONY : all clean

all: $(TESTUNIT)

%.o:%.c
	$(QUIET_CC) $(COMPILE_SRC) $< -o $@

$(TESTUNIT):$(TESTUNIT_OBJS)
	$(QUIET_LINK)$(LINK_OBJS) -o $(OUTDIR)/$@ $(TESTUNIT_OBJS) $(LDFLAGS)

clean:
	@rm -rf $(TESTUNIT_OBJS)