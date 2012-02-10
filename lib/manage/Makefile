MODULE_TOPDIR = ../..

LIB = MANAGE
DEPENDENCIES = $(GISDEP) $(VECTORDEP) $(RASTER3DDEP)

include $(MODULE_TOPDIR)/include/Make/Lib.make
include $(MODULE_TOPDIR)/include/Make/Doxygen.make

EXTRA_INC = $(VECT_INC)
EXTRA_CFLAGS = $(VECT_CFLAGS)

default: lib $(ETC)/element_list

$(ETC)/%: %
	$(INSTALL_DATA) $< $@

DOXNAME = manage