MODULE_TOPDIR = ../../..

PGM = i.ortho.target

EXTRA_CFLAGS = -I../lib

LIBES     = $(IMAGERYLIB) $(GISLIB) $(IORTHOLIB)
DEPENDENCIES= $(IMAGERYDEP) $(IORTHODEP) $(GISDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

default: cmd

htmletc:
	@echo No docs needed for $(PGM)
