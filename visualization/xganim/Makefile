MODULE_TOPDIR = ../..

PGM = xganim

EXTRA_CFLAGS = $(WXWIDGETSCXXFLAGS) -DWXINTL_NO_GETTEXT_MACRO

LIBES = $(GISLIB) $(RASTRLIB) $(WXWIDGETSLIB)
DEPENDENCIES = $(GISDEP) $(RASTRDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

LINK = $(CXX)

ifneq ($(USE_WXWIDGETS),)
ifneq ($(strip $(CXX)),)
default: cmd
endif
endif
