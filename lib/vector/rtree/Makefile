MODULE_TOPDIR = ../../..

LIB = RTREE

include $(MODULE_TOPDIR)/include/Make/Lib.make

RTLINC = $(ARCH_INCDIR)/rtree
HEADERS := $(RTLINC)/card.h $(RTLINC)/index.h $(RTLINC)/split.h \
	$(ARCH_INCDIR)/rtree.h

default: headers
	$(MAKE) lib

headers: $(HEADERS)

$(RTLINC):
	$(MKDIR) $@

$(RTLINC)/%.h: %.h | $(RTLINC)
	$(INSTALL_DATA) $< $@

$(ARCH_INCDIR)/rtree.h: rtree.h
	$(INSTALL_DATA) $< $@
