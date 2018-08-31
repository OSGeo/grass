MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Vars.make

MOD_OBJS := $(filter-out test.o port_test.o,$(AUTO_OBJS))

LIB = CDHC

include $(MODULE_TOPDIR)/include/Make/Lib.make
include $(MODULE_TOPDIR)/include/Make/Doxygen.make

default: lib
	@echo "==============TEST============="
ifeq ($(strip $(CROSS_COMPILING)),)
	$(MAKE) test
endif

test: $(OBJDIR)/test$(EXE)
	$(call run_grass,$(OBJDIR)/test$(EXE) < test_numbers.csv)

#  Test functions
$(OBJDIR)/test$(EXE): $(OBJDIR)/test.o $(GISDEP) $(CDHCDEP)
	$(call linker)

$(OBJDIR)/test$(EXE): LIBES = $(GISLIB) $(CDHCLIB)

DOXNAME = cdhc
