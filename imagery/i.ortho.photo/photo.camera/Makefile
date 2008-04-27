MODULE_TOPDIR = ../../..

PGM = photo.camera

EXTRA_CFLAGS = -I../libes

LIBES     = $(IORTHOLIB) $(IMAGERYLIB) $(GISLIB) $(VASKLIB) $(CURSES) $(GMATHLIB)
DEPENDENCIES= $(IMAGERYDEP) $(IORTHODEP) $(GISDEP) $(VASKDEP) $(GMATHDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

default: etc

htmletc:
	$(MKDIR) $(GISBASE)/docs/html
	$(INSTALL_DATA) description.html $(GISBASE)/docs/html/$(PGM).html
	$(INSTALL_DATA) *.png $(GISBASE)/docs/html/
