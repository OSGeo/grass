MODULE_TOPDIR = ../../..

LIBES = $(VECTLIB) $(DBMILIB) $(GISLIB) 
DEPENDENCIES = $(VECTDEP) $(DBMIDEP) $(GISDEP)
EXTRA_INC = $(VECT_INC)
EXTRA_CFLAGS = $(VECT_CFLAGS)

include $(MODULE_TOPDIR)/include/Make/Module.make

PGM1=cell
PGM2=vector

default: $(ETC)/lister/cell $(ETC)/lister/vector

ifneq ($(BROKEN_MAKE),)
$(ETC)/lister/cell: $(OBJDIR)/cell.o $(ETC)/lister
else
$(ETC)/lister/cell: $(OBJDIR)/cell.o | $(ETC)/lister
endif
	$(CC) $(LDFLAGS) -o $@ $(OBJDIR)/cell.o $(FMODE_OBJ) $(LIST) $(LIBES) $(MATHLIB) $(XDRLIB)

ifneq ($(BROKEN_MAKE),)
$(ETC)/lister/vector: $(OBJDIR)/vector.o $(ETC)/lister
else
$(ETC)/lister/vector: $(OBJDIR)/vector.o | $(ETC)/lister
endif
	$(CC) $(LDFLAGS) -o $@ $(OBJDIR)/vector.o $(FMODE_OBJ) $(LIST) $(LIBES) $(MATHLIB) $(XDRLIB)

$(ETC)/lister:
	$(MKDIR) $(ETC)/lister
