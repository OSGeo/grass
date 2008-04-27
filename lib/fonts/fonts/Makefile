
MODULE_TOPDIR = ../../..

include $(MODULE_TOPDIR)/include/Make/Platform.make
include $(MODULE_TOPDIR)/include/Make/Grass.make
include $(MODULE_TOPDIR)/include/Make/Rules.make

SRCFONTS := $(wildcard hersh.oc?)
SRCMAPS := $(wildcard *.hmp)
SRCTABLE = fonts.table

DSTFONTS := $(patsubst %,$(FONTDIR)/%,$(SRCFONTS))
DSTMAPS := $(patsubst %,$(FONTDIR)/%,$(SRCMAPS))
DSTTABLE := $(patsubst %,$(FONTDIR)/%,$(SRCTABLE))

default: $(DSTFONTS) $(DSTMAPS) $(DSTTABLE)

$(FONTDIR)/%: %
	$(INSTALL_DATA) $^ $(FONTDIR)
