MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Platform.make

EXTRA_CFLAGS=-I../driver

EXTRA_LIBS=$(DRIVERLIB) $(GISLIB)
LIB_NAME = $(PSDRIVER_LIBNAME)
PGM = psdriver

include $(MODULE_TOPDIR)/include/Make/Lib.make

default: lib htmldir $(ETC)/psdriver.ps

$(ETC)/psdriver.ps: psdriver.ps
	$(INSTALL_DATA) $< $@
