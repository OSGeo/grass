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


default: cmd $(PROLOG) $(DSTPAT) $(DSTDEC)

$(PROLOG): prolog.ps | $(ETC)/paint
	$(INSTALL_DATA) $< $@

$(ETC)/paint/patterns/%.eps: patterns/%.eps | $(ETC)/paint/patterns
	$(INSTALL_DATA) $< $@

$(ETC)/paint/decorations/%.eps: decorations/%.eps | $(ETC)/paint/decorations
	$(INSTALL_DATA) $< $@

$(ETC)/paint:
	$(MKDIR) $@

$(ETC)/paint/patterns: | $(ETC)/paint
	$(MKDIR) $@

$(ETC)/paint/decorations: | $(ETC)/paint
	$(MKDIR) $@
