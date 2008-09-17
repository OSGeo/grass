MODULE_TOPDIR = ../..

PGM=ps.map
PROLOG=$(ETC)/paint/prolog.ps

LIBES     = $(VECTLIB) $(GPROJLIB) $(SYMBLIB) $(GISLIB) $(DATETIMELIB) $(IMAGERYLIB) $(GMATHLIB)
DEPENDENCIES = $(VECTDEP) $(SYMBDEP) $(GISDEP) $(DATETIMEDEP) $(IMAGERYDEP) $(GMATHDEP)

EXTRA_INC = $(VECT_INC) $(PROJINC)
EXTRA_CFLAGS = $(VECT_CFLAGS)

include $(MODULE_TOPDIR)/include/Make/Module.make

SRCPAT := $(wildcard patterns/*.eps)
DSTPAT := $(patsubst patterns/%.eps,$(ETC)/paint/patterns/%.eps,$(SRCPAT))

default: cmd $(PROLOG) patt

$(PROLOG): prolog.ps
	$(MKDIR) $(ETC)/paint
	$(INSTALL_DATA) $< $@

patt:
	$(MKDIR) $(ETC)/paint/patterns/
	$(MAKE) $(DSTPAT)

$(ETC)/paint/patterns/%.eps: patterns/%.eps
	$(INSTALL_DATA) $< $@
