MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Platform.make

PACKAGE ="libhtmldriver"
DEFS=-DPACKAGE=\"$(PACKAGE)\"
EXTRA_CFLAGS=-I../driver $(PICFLAGS) $(DEFS)

EXTRA_LIBS=$(DRIVERLIB) $(GISLIB)
LIB_NAME = $(HTMLDRIVER_LIBNAME)

include $(MODULE_TOPDIR)/include/Make/Lib.make

default: lib
