MODULE_TOPDIR = ../../..

PGM = photo.2target

EXTRA_CFLAGS = -I../libes

LIBES     = $(IMAGERYLIB) $(DISPLAYLIB) $(RASTERLIB) $(GISLIB) $(IORTHOLIB) $(VASKLIB) $(CURSES) $(GMATHLIB)
DEPENDENCIES= $(IMAGERYDEP) $(DISPLAYDEP) $(RASTERDEP) $(IORTHODEP) $(GISDEP) $(VASKDEP) $(GMATHDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

default: etc

$(PGM).tmp.html: $(ETC)/$(PGM)$(EXE)
	$(call htmldesc,$(ETC)/$(PGM)$(EXE))
