TOPDIR ?= ../../..
#CROSS_COMPILE ?=

include ../../../config.mk

TESTUNIT := test_libmxml
TESTUNIT_OBJS := main.o                                                        \
          $(TOPDIR)/lib/mxml/mxml-2.10/mxml-attr.o                             \
          $(TOPDIR)/lib/mxml/mxml-2.10/mxml-entity.o                           \
          $(TOPDIR)/lib/mxml/mxml-2.10/mxml-file.o                             \
          $(TOPDIR)/lib/mxml/mxml-2.10/mxml-get.o                              \
          $(TOPDIR)/lib/mxml/mxml-2.10/mxml-index.o                            \
          $(TOPDIR)/lib/mxml/mxml-2.10/mxml-node.o                             \
          $(TOPDIR)/lib/mxml/mxml-2.10/mxml-search.o                           \
          $(TOPDIR)/lib/mxml/mxml-2.10/mxml-set.o                              \
          $(TOPDIR)/lib/mxml/mxml-2.10/mxml-private.o                          \
          $(TOPDIR)/lib/mxml/mxml-2.10/mxml-string.o

.PHONY : all clean

all: $(TESTUNIT)

$(TESTUNIT): $(TESTUNIT_OBJS)
	$(QUIET_LINK)$(LINK_OBJS) -o $(OUTDIR)/$@ $(TESTUNIT_OBJS) $(LDFLAGS) $(LDLIBS)

clean:
	rm -rf $(TESTUNIT_OBJS)
