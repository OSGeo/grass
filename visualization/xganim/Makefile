MODULE_TOPDIR = ../..

PGM = xganim

EXTRA_CFLAGS = $(WXWIDGETSCXXFLAGS) -DWXINTL_NO_GETTEXT_MACRO

LIBES = $(GISLIB) $(RASTERLIB) $(WXWIDGETSLIB)
DEPENDENCIES = $(GISDEP) $(RASTERDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

LINK = $(CXX)

ifneq ($(USE_WXWIDGETS),)
ifneq ($(strip $(CXX)),)
default: cmd
endif
endif
