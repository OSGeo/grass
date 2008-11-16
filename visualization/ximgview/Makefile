MODULE_TOPDIR = ../..

PGM = ximgview

EXTRA_CFLAGS=$(XCFLAGS) $(XMINC)

LIBES = $(GISLIB) $(XLIBPATH) $(XLIB) $(XEXTRALIBS)
DEPENDENCIES = $(GISDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

ifneq ($(USE_X11),)
default: cmd
endif
