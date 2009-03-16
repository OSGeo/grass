MODULE_TOPDIR = ../../..

LIB_NAME = $(RTREE_LIBNAME)

include $(MODULE_TOPDIR)/include/Make/Lib.make

EXTRA_LIBS = $(MATHLIB)

#compile if LFS (Large File Support) present:
ifneq ($(USE_LARGEFILES),)
	EXTRA_CFLAGS += -D_FILE_OFFSET_BITS=64
endif

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
	$(CC) $(CFLAGS) $(EXTRA_FLAGS) $(LDFLAGS) $(INC) -o $@ test.c $(RTREELIB) -lm
	cd $(OBJDIR) && ./test

