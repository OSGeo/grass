MODULE_TOPDIR = ../..

EXTRA_CFLAGS=-I../driver

EXTRA_LIBS=$(DRIVERLIB) $(GISLIB) $(MATHLIB)
LIB_NAME = $(PSDRIVER_LIBNAME)
PGM = psdriver

include $(MODULE_TOPDIR)/include/Make/Lib.make

default: lib $(ETC)/psdriver.ps

$(ETC)/psdriver.ps: psdriver.ps
	$(INSTALL_DATA) $< $@
