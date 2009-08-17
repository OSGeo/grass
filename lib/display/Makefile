MODULE_TOPDIR = ../..

EXTRA_LIBS=$(GISLIB) $(DRIVERLIB) $(PNGDRIVERLIB) $(PSDRIVERLIB) \
	$(HTMLDRIVERLIB) $(RASTERLIB)
EXTRA_INC = -I../driver

include $(MODULE_TOPDIR)/include/Make/Vars.make
ifneq ($(USE_CAIRO),)
EXTRA_LIBS +=  $(CAIRODRIVERLIB)
EXTRA_CFLAGS += -DUSE_CAIRO
endif

LIB_NAME = $(DISPLAY_LIBNAME)

include $(MODULE_TOPDIR)/include/Make/Lib.make
include $(MODULE_TOPDIR)/include/Make/Doxygen.make

default: lib

#doxygen:
DOXNAME=display
