MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Platform.make

EXTRA_CFLAGS=-I../driver

EXTRA_LIBS=$(DRIVERLIB) $(GISLIB)
LIB_NAME = $(HTMLDRIVER_LIBNAME)
PGM = htmldriver

include $(MODULE_TOPDIR)/include/Make/Lib.make

default: lib htmldir
