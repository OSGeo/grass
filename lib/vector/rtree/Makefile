MODULE_TOPDIR = ../../..

LIB_NAME = $(RTREE_LIBNAME)

LIB_OBJS = card.o \
	gammavol.o \
	index.o \
	node.o \
	rect.o \
	split_q.o 

EXTRA_LIBS = $(MATHLIB)

include $(MODULE_TOPDIR)/include/Make/Lib.make

RTLINC = $(ARCH_INCDIR)/rtree
HEADERS := $(RTLINC)/card.h $(RTLINC)/index.h $(RTLINC)/split_q.h \
	$(ARCH_INCDIR)/rtree.h

default: $(HEADERS)
	$(MAKE) lib

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

