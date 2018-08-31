MODULE_TOPDIR = ../..

EXTRA_CFLAGS = -I../driver

LIB = PSDRIVER
PGM = psdriver

include $(MODULE_TOPDIR)/include/Make/Lib.make
include $(MODULE_TOPDIR)/include/Make/Doxygen.make

default: lib $(ETC)/psdriver.ps

$(ETC)/psdriver.ps: psdriver.ps
	$(INSTALL_DATA) $< $@

DOXNAME = psdriver
