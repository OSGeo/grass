MODULE_TOPDIR = ../..

PGM = g.remove

LIBES = $(MANAGELIB) $(RASTERLIB) $(GISLIB)
DEPENDENCIES = $(MANAGEDEP) $(RASTERDEP) $(GISDEP)
EXTRA_INC = $(VECT_INC)
EXTRA_CFLAGS = $(VECT_CFLAGS)

include $(MODULE_TOPDIR)/include/Make/Module.make

ifneq ($(USE_REGEX),)
default: cmd
endif
