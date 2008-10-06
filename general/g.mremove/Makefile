MODULE_TOPDIR = ../..

PGM = g.mremove

LIBES = $(GISLIB) $(VECTLIB) $(G3DLIB) $(REGEXLIBPATH) $(REGEXLIB)
DEPENDENCIES = $(GISDEP) $(VECTDEP) $(G3DDEP)
EXTRA_INC = $(VECT_INC) $(REGEXINCPATH)
EXTRA_CFLAGS = $(VECT_CFLAGS)

include $(MODULE_TOPDIR)/include/Make/Module.make

ifneq ($(USE_REGEX),)
default: cmd
else
default:
endif
