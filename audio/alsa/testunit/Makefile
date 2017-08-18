TOPDIR ?= ../../..

include ../../../config.mk

TESTUNIT := test_wave_play
TESTUNIT_OBJS := test_wave_play.o

TESTUNIT2 := test_wave_record
TESTUNIT2_OBJS := test_wave_record.o

LINK_LIBS := -L$(LIBS_OUTDIR) -l$(TARGET_NAME)

.PHONY : all clean

all: $(TESTUNIT) $(TESTUNIT2)

%.o:%.c
	$(QUIET_CC) $(COMPILE_SRC) $< -o $@

$(TESTUNIT):$(TESTUNIT_OBJS)
	$(QUIET_LINK)$(LINK_OBJS) $(LINK_LIBS) -o $(OUTDIR)/$@ $(TESTUNIT_OBJS) $(LDFLAGS)

$(TESTUNIT2):$(TESTUNIT2_OBJS)
	$(QUIET_LINK)$(LINK_OBJS) $(LINK_LIBS) -o $(OUTDIR)/$@ $(TESTUNIT2_OBJS) $(LDFLAGS)

clean:
	@rm -rf $(TESTUNIT_OBJS) $(TESTUNIT2_OBJS)
