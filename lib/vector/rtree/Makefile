MODULE_TOPDIR = ../../..

LIB_NAME = $(RTREE_LIBNAME)

include $(MODULE_TOPDIR)/include/Make/Lib.make

EXTRA_LIBS = $(MATHLIB)

RTLINC = $(ARCH_INCDIR)/rtree
HEADERS := $(RTLINC)/card.h $(RTLINC)/index.h $(RTLINC)/split_q.h \
	$(ARCH_INCDIR)/rtree.h

default: headers
	$(MAKE) lib

headers: $(HEADERS)

$(RTLINC):
	$(MKDIR) $@

ifneq ($(BROKEN_MAKE),)
$(RTLINC)/%.h: %.h $(RTLINC)
else
$(RTLINC)/%.h: %.h | $(RTLINC)
endif
	$(INSTALL_DATA) $< $@

$(ARCH_INCDIR)/rtree.h: rtree.h
	$(INSTALL_DATA) $< $@

$(OBJDIR)/test: test.c
	$(CC) $(LDFLAGS) $(INC) -o $@ test.c $(RTREELIB) -lm
	cd $(OBJDIR) && ./test

