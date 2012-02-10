MODULE_TOPDIR = ../..

PGM = wximgview

EXTRA_CFLAGS = $(WXWIDGETSCXXFLAGS) -DWXINTL_NO_GETTEXT_MACRO

LIBES = $(GISLIB) $(WXWIDGETSLIB)
DEPENDENCIES = $(GISDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

# substitute OSX arch flags for wxpython
ifneq ($(MACOSX_ARCHS),)
CFLAGS := $(subst $(MACOSX_ARCHS),,$(CFLAGS)) $(MACOSX_ARCHS_WXPYTHON)
CXXFLAGS := $(subst $(MACOSX_ARCHS),,$(CXXFLAGS)) $(MACOSX_ARCHS_WXPYTHON)
LDFLAGS := $(subst $(MACOSX_ARCHS),,$(LDFLAGS)) $(MACOSX_ARCHS_WXPYTHON)
endif

LINK = $(CXX)

ifneq ($(USE_WXWIDGETS),)
ifneq ($(strip $(CXX)),)
default: cmd
endif
endif
