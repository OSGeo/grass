MODULE_TOPDIR = ../../..

LIBES = $(VECTORLIB) $(DBMILIB) $(GISLIB) $(RASTERLIB)
DEPENDENCIES = $(VECTORDEP) $(DBMIDEP) $(GISDEP) $(RASTERDEP)
EXTRA_INC = $(VECT_INC)
EXTRA_CFLAGS = $(VECT_CFLAGS)

include $(MODULE_TOPDIR)/include/Make/Module.make

default: $(ETC)/lister/cell $(ETC)/lister/vector

$(ETC)/lister/%: $(OBJDIR)/%.o | $(ETC)/lister
	$(call linker)

$(ETC)/lister:
	$(MKDIR) $@
