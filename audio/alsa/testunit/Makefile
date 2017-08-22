TOPDIR ?= ../../..

include ../../../config.mk

TESTUNIT := test_wave_play
TESTUNIT_OBJS := test_wave_play.o

TESTUNIT2 := test_wave_record
TESTUNIT2_OBJS := test_wave_record.o

TESTUNIT3 := test_mixer_controller
TESTUNIT3_OBJS := test_mixer_controller.o

LINK_LIBS := -L$(LIBS_OUTDIR) -l$(TARGET_NAME)

.PHONY : all clean

all: $(TESTUNIT) $(TESTUNIT2) $(TESTUNIT3)

%.o:%.c
	$(QUIET_CC) $(COMPILE_SRC) $< -o $@

$(TESTUNIT):$(TESTUNIT_OBJS)
	$(QUIET_LINK)$(LINK_OBJS) $(LINK_LIBS) -o $(OUTDIR)/$@ $(TESTUNIT_OBJS) $(LDFLAGS)

$(TESTUNIT2):$(TESTUNIT2_OBJS)
	$(QUIET_LINK)$(LINK_OBJS) $(LINK_LIBS) -o $(OUTDIR)/$@ $(TESTUNIT2_OBJS) $(LDFLAGS)

$(TESTUNIT3):$(TESTUNIT3_OBJS)
	$(QUIET_LINK)$(LINK_OBJS) $(LINK_LIBS) -o $(OUTDIR)/$@ $(TESTUNIT3_OBJS) $(LDFLAGS)

clean:
	@rm -rf $(TESTUNIT_OBJS) $(TESTUNIT2_OBJS) $(TESTUNIT3_OBJS)
