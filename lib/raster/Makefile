MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Vars.make

EXTRA_CFLAGS = -I../driver
EXTRA_LIBS=$(DRIVERLIB) $(GISLIB) $(PNGDRIVERLIB) $(PSDRIVERLIB) $(HTMLDRIVERLIB)
LIB_NAME = $(RASTER_LIBNAME)

ifneq ($(USE_CAIRO),)
EXTRA_LIBS +=  $(CAIRODRIVERLIB)
EXTRA_CFLAGS += -DUSE_CAIRO
endif

include $(MODULE_TOPDIR)/include/Make/Lib.make
include $(MODULE_TOPDIR)/include/Make/Doxygen.make

default: lib

#doxygen:
DOXNAME=rastergraphics
