MODULE_TOPDIR = ../..

LIBES   = $(DISPLAYLIB) $(RASTERLIB) $(GISLIB)
DEPLIBS = $(DISPLAYDEP) $(RASTERDEP) $(GISDEP)

PGM=d.frame

include $(MODULE_TOPDIR)/include/Make/Module.make

default: \
	$(ETC)/frame.create \
	$(ETC)/frame.select \
	$(ETC)/frame.dumper \
	$(ETC)/frame.list \
	$(BIN)/$(PGM)$(EXE) \
	cmd

$(ETC)/frame.create: $(OBJDIR)/create.o $(DEPLIBS)
	$(CC) $(LDFLAGS) -o $@ $(OBJDIR)/create.o $(LIBES) $(MATHLIB) $(XDRLIB)

$(ETC)/frame.select: $(OBJDIR)/select.o $(DEPLIBS)
	$(CC) $(LDFLAGS) -o $@ $(OBJDIR)/select.o $(LIBES) $(MATHLIB) $(XDRLIB)

$(ETC)/frame.dumper: $(OBJDIR)/dumper.o $(DEPLIBS)
	$(CC) $(LDFLAGS) -o $@ $(OBJDIR)/dumper.o $(LIBES) $(MATHLIB) $(XDRLIB)

$(ETC)/frame.list: $(OBJDIR)/list.o $(DEPLIBS)
	$(CC) $(LDFLAGS) -o $@ $(OBJDIR)/list.o $(LIBES) $(MATHLIB) $(XDRLIB)

$(BIN)/$(PGM)$(EXE): $(OBJDIR)/frame.o $(DEPLIBS)
	$(CC) $(LDFLAGS) -o $@ $(OBJDIR)/frame.o $(LIBES) $(MATHLIB) $(XDRLIB)
