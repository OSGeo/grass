MODULE_TOPDIR = ../..

PGM = g.mlist

LIBES = $(GISLIB)
DEPENDENCIES = $(GISDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

ifneq ($(USE_REGEX),)
default: cmd
else
default:
endif
