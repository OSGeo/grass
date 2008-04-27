MODULE_TOPDIR = ../../..

PGM = photo.init

EXTRA_CFLAGS = -I../libes

LIBES     = $(IORTHOLIB) $(IMAGERYLIB) $(DISPLAYLIB) $(RASTERLIB) $(GISLIB) $(VASKLIB) $(CURSES) $(GMATHLIB)
DEPENDENCIES= $(IMAGERYDEP) $(DISPLAYDEP) $(RASTERDEP) $(IORTHODEP) $(GISDEP) $(VASKDEP) $(GMATHDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

default: etc

$(PGM).tmp.html: description.html
	$(INSTALL_DATA) $< $@

