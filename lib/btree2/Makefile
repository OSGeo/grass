MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Vars.make

MOD_OBJS := $(filter-out test.o,$(AUTO_OBJS))

LIB = BTREE2

include $(MODULE_TOPDIR)/include/Make/Lib.make
include $(MODULE_TOPDIR)/include/Make/Doxygen.make

HEADERS := $(ARCH_INCDIR)/kdtree.h

headers: $(HEADERS)

default: lib headers
	$(MAKE) lib
	@echo "==============TEST============="
ifeq ($(strip $(CROSS_COMPILING)),)
	$(MAKE) test
endif

$(ARCH_INCDIR)/kdtree.h: kdtree.h
	$(INSTALL_DATA) $< $@

test: $(OBJDIR)/test$(EXE)
	$(call run_grass,$(OBJDIR)/test$(EXE))

#  Test functions
$(OBJDIR)/test$(EXE): $(OBJDIR)/test.o $(GISDEP) $(BTREE2DEP)
	$(call linker)

$(OBJDIR)/test$(EXE): LIBES = $(GISLIB) $(BTREE2LIB)

DOXNAME = btree2
