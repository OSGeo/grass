MODULE_TOPDIR = ../../..

LIBES = $(VECTLIB) $(DBMILIB) $(GISLIB) $(RASTERLIB)
DEPENDENCIES = $(VECTDEP) $(DBMIDEP) $(GISDEP) $(RASTERDEP)
EXTRA_INC = $(VECT_INC)
EXTRA_CFLAGS = $(VECT_CFLAGS)

include $(MODULE_TOPDIR)/include/Make/Module.make

default: $(ETC)/lister/cell $(ETC)/lister/vector

$(ETC)/lister/%: $(OBJDIR)/%.o | $(ETC)/lister
	$(CC) $(LDFLAGS) -o $@ $< $(FMODE_OBJ) $(LIST) $(LIBES) $(MATHLIB) $(XDRLIB)

$(ETC)/lister:
	$(MKDIR) $@
