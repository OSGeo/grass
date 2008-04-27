MODULE_TOPDIR = ../..

PACKAGE ="libraster"
DEFS=-DPACKAGE=\"$(PACKAGE)\"
EXTRA_CFLAGS=$(GETHOSTNAME) $(FTINC) -I../driver $(PICFLAGS) $(DEFS)

EXTRA_LIBS=$(DRIVERLIB) $(GISLIB) $(PNGDRIVERLIB) $(PSDRIVERLIB)
LIB_NAME = $(RASTER_LIBNAME)

include $(MODULE_TOPDIR)/include/Make/Lib.make
include $(MODULE_TOPDIR)/include/Make/Doxygen.make

default: lib

#doxygen:
DOXNAME=rastergraphics
