MODULE_TOPDIR = ../../..

PGM = photo.2image

EXTRA_CFLAGS = -I../libes

LIBES     = $(IMAGERYLIB) $(DISPLAYLIB) $(RASTERLIB) $(GISLIB) $(IORTHOLIB) $(VASKLIB) $(CURSES) $(GMATHLIB)
DEPENDENCIES= $(IMAGERYDEP) $(DISPLAYDEP) $(RASTERDEP) $(IORTHODEP) $(GISDEP) $(VASKDEP) $(GMATHDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

default: etc

$(PGM).tmp.html: description.html
	$(INSTALL_DATA) $< $@

