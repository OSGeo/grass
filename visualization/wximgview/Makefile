MODULE_TOPDIR = ../..

PGM = wximgview

EXTRA_CFLAGS = $(WXWIDGETSCXXFLAGS) -DWXINTL_NO_GETTEXT_MACRO

LIBES = $(RASTERLIB) $(GISLIB) $(WXWIDGETSLIB)
DEPENDENCIES = $(RASTERDEP) $(GISDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

LINK = $(CXX)

ifneq ($(USE_WXWIDGETS),)
ifneq ($(strip $(CXX)),)
default: cmd
endif
endif
