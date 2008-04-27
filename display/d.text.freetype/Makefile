MODULE_TOPDIR = ../..

PGM = d.text.freetype
CAPFILE=freetypecap
EXTRA_CFLAGS=$(FTINC)

LIBES     = $(DISPLAYLIB) $(RASTERLIB) $(GISLIB) $(FTLIB) $(ICONVLIB)
DEPENDENCIES= $(DISPLAYDEP) $(RASTERDEP) $(GISDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

default: cmd $(ETC)/$(CAPFILE)

$(ETC)/$(CAPFILE): $(CAPFILE)
	$(INSTALL_DATA) $< $@

