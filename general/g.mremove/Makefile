MODULE_TOPDIR = ../..

PGM = g.mremove

LIBES = $(MANAGELIB) $(VECTLIB) $(G3DLIB) $(GISLIB) $(REGEXLIBPATH) $(REGEXLIB)
DEPENDENCIES = $(MANAGEDEP) $(VECTDEP) $(G3DDEP) $(GISDEP)
EXTRA_INC = $(VECT_INC) $(REGEXINCPATH)
EXTRA_CFLAGS = $(VECT_CFLAGS)

include $(MODULE_TOPDIR)/include/Make/Module.make

ifneq ($(USE_REGEX),)
default: cmd
else
default:
endif
