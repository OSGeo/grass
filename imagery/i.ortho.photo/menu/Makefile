MODULE_TOPDIR = ../../..

PGM = i.ortho.photo

EXTRA_CFLAGS = -I../libes

LIBES     = $(IMAGERYLIB) $(GISLIB) $(IORTHOLIB) $(VASKLIB) $(CURSES) $(GMATHLIB)
DEPENDENCIES= $(IMAGERYDEP) $(IORTHODEP) $(GISDEP) $(VASKDEP) $(GMATHDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

default: cmd
