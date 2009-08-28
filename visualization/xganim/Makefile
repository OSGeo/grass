MODULE_TOPDIR = ../..

PGM = xganim

EXTRA_CFLAGS = $(WXWIDGETSCXXFLAGS) -DWXINTL_NO_GETTEXT_MACRO

LIBES = $(RASTERLIB) $(GISLIB) $(WXWIDGETSLIB)
DEPENDENCIES = $(RASTERDEP) $(GISDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

ifeq ($(findstring darwin,$(ARCH)),darwin)
ifneq ($(WX_ISOSX64BIT),1)
CFLAGS := $(subst -arch ppc64,,$(subst -arch x86_64,,$(CFLAGS)))
LDFLAGS := $(subst -arch ppc64,,$(subst -arch x86_64,,$(LDFLAGS)))
endif
endif

LINK = $(CXX)

ifneq ($(USE_WXWIDGETS),)
ifneq ($(strip $(CXX)),)
default: cmd
endif
endif
