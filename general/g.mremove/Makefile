MODULE_TOPDIR = ../..

PGM = g.mremove

LIBES = $(GISLIB) $(VECTLIB) $(G3DLIB)
DEPENDENCIES = $(GISDEP) $(VECTDEP) $(G3DDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

ifneq ($(USE_REGEX),)
default: cmd
else
default:
endif
