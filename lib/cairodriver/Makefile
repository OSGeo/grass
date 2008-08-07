MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Platform.make

PACKAGE ="libcairodriver"
DEFS=-DPACKAGE=\"$(PACKAGE)\"

EXTRA_CFLAGS=-I../driver $(CAIROINC) $(PICFLAGS) $(DEFS)
EXTRA_LIBS=$(DRIVERLIB) $(GISLIB) $(CAIROLIB)

# added constant to /include/Make/Grass.make.in
LIB_NAME = $(CAIRODRIVER_LIBNAME)

include $(MODULE_TOPDIR)/include/Make/Lib.make

ifneq ($(USE_CAIRO),)
default: lib
else
default:
endif

