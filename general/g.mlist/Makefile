MODULE_TOPDIR = ../..

PGM = g.mlist

LIBES = $(MANAGELIB) $(GISLIB)
DEPENDENCIES = $(MANAGEDEP) $(GISDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

ifneq ($(USE_REGEX),)
default: cmd
endif
