TOPDIR ?= ../..

include ../../config.mk

TESTUNIT := test_gpio
TESTUNIT_OBJS := main.o
LINK_LIBS := -L$(OUTDIR) -l$(TARGET_NAME)

.PHONY : all clean

all: $(TESTUNIT)

%.o:%.c
	$(QUIET_CC) $(COMPILE_SRC) $< -o $@

$(TESTUNIT):$(TESTUNIT_OBJS)
	$(QUIET_LINK)$(LINK_OBJS) $(LINK_LIBS) $(LDFLAGS) -o $(OUTDIR)/$@  $(TESTUNIT_OBJS)

clean:
	@rm -rf $(TESTUNIT_OBJS)
