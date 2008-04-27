MODULE_TOPDIR = ../../..

PGM = photo.rectify

# for DEBUG3 see README!
#EXTRA_CFLAGS = -I../libes -DDEBUG3
EXTRA_CFLAGS = -I../libes

LIBES     = $(IMAGERYLIB) $(GISLIB) $(IORTHOLIB) $(VASKLIB) $(CURSES) $(GMATHLIB)
DEPENDENCIES= $(IMAGERYDEP) $(IORTHODEP) $(GISDEP) $(VASKDEP) $(GMATHDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

default: etc

$(PGM).tmp.html: description.html
	$(INSTALL_DATA) $< $@

