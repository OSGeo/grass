MODULE_TOPDIR = ../..

PGM=ps.map
PROLOG=$(ETC)/paint/prolog.ps

LIBES = $(GPROJLIB) $(SYMBLIB) $(IMAGERYLIB) $(VECTORLIB) $(DBMILIB) $(RASTERLIB) $(GISLIB) $(MATHLIB)
DEPENDENCIES = $(GPROJDEP) $(SYMBDEP) $(IMAGERYDEP) $(VECTORDEP) $(DBMIDEP) $(RASTERDEP) $(GISDEP)

EXTRA_INC = $(VECT_INC) $(PROJINC)
EXTRA_CFLAGS = $(VECT_CFLAGS)

include $(MODULE_TOPDIR)/include/Make/Module.make

SRCPAT := $(wildcard patterns/*.eps)
DSTPAT := $(patsubst patterns/%.eps,$(ETC)/paint/patterns/%.eps,$(SRCPAT))
SRCDEC := $(wildcard decorations/*.eps)
DSTDEC := $(patsubst decorations/%.eps,$(ETC)/paint/decorations/%.eps,$(SRCDEC))


default: cmd $(PROLOG) patt decor

$(PROLOG): prolog.ps
	if [ ! -d $(ETC)/paint ] ; then $(MKDIR) $(ETC)/paint ; fi
	$(INSTALL_DATA) $< $@

patt:
	if [ ! -d $(ETC)/paint/patterns ] ; then $(MKDIR) $(ETC)/paint/patterns ; fi
	$(MAKE) $(DSTPAT)

$(ETC)/paint/patterns/%.eps: patterns/%.eps
	$(INSTALL_DATA) $< $@

decor:
	if [ ! -d $(ETC)/paint/decorations ] ; then $(MKDIR) $(ETC)/paint/decorations ; fi
	$(MAKE) $(DSTDEC)

$(ETC)/paint/decorations/%.eps: decorations/%.eps
	$(INSTALL_DATA) $< $@


.PHONY: patt decor
