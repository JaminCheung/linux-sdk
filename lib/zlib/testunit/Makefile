TOPDIR ?= ../../..
#CROSS_COMPILE ?=

include ../../../config.mk

TESTUNIT := test_zlib
TESTUNIT_OBJS := main.o                                                        \
          $(TOPDIR)/lib/zlib/zlib-1.2.8/adler32.o                              \
          $(TOPDIR)/lib/zlib/zlib-1.2.8/crc32.o                                \
          $(TOPDIR)/lib/zlib/zlib-1.2.8/deflate.o                              \
          $(TOPDIR)/lib/zlib/zlib-1.2.8/infback.o                              \
          $(TOPDIR)/lib/zlib/zlib-1.2.8/inffast.o                              \
          $(TOPDIR)/lib/zlib/zlib-1.2.8/inflate.o                              \
          $(TOPDIR)/lib/zlib/zlib-1.2.8/inftrees.o                             \
          $(TOPDIR)/lib/zlib/zlib-1.2.8/trees.o                                \
          $(TOPDIR)/lib/zlib/zlib-1.2.8/zutil.o                                \
          $(TOPDIR)/lib/zlib/zlib-1.2.8/compress.o                             \
          $(TOPDIR)/lib/zlib/zlib-1.2.8/uncompr.o                              \
#          $(TOPDIR)/lib/zlib/zlib-1.2.8/gzclose.o                              \
          $(TOPDIR)/lib/zlib/zlib-1.2.8/gzlib.o                                \
          $(TOPDIR)/lib/zlib/zlib-1.2.8/gzread.o                               \
          $(TOPDIR)/lib/zlib/zlib-1.2.8/gzwrite.o

.PHONY : all clean

all: $(TESTUNIT)

$(TESTUNIT): $(TESTUNIT_OBJS)
	$(QUIET_LINK)$(LINK_OBJS) -o $(OUTDIR)/$@ $(TESTUNIT_OBJS) $(LDFLAGS) $(LDLIBS)

clean:
	rm -rf $(TESTUNIT_OBJS)
