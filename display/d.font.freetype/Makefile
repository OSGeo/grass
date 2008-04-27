MODULE_TOPDIR = ../..

PGM = d.font.freetype
EXTRA_CFLAGS=$(FTINC)

LIBES     = $(DISPLAYLIB) $(RASTERLIB) $(GISLIB) $(FTLIB) $(ICONVLIB)
DEPENDENCIES= $(DISPLAYDEP) $(RASTERDEP) $(GISDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

default: cmd
