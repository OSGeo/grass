
MODULE_TOPDIR = ../..

PGM = g.cairocomp
EXTRA_INC = $(CAIROINC)
LIBES = $(GISLIB) $(CAIROLIB)
DEPENDENCIES= $(GISDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

ifneq ($(USE_CAIRO),)
ifneq ($(USE_X11),)
ifneq ($(CAIRO_HAS_XRENDER),)
default: cmd
endif
endif
endif
