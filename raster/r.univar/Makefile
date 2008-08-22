
MODULE_TOPDIR = ../..

LIBES = $(G3DLIB) $(GISLIB)
DEPENDENCIES = $(G3DDEP) $(GISDEP)

PROGRAMS = r.univar r3.univar

include $(MODULE_TOPDIR)/include/Make/Multi.make

R3UNIVAR = $(BIN)/r3.univar$(EXE)
RUNIVAR = $(BIN)/r.univar$(EXE)

default: multi

$(BIN)/%$(EXE): $(OBJDIR)/%_main.o $(OBJDIR)/sort.o $(OBJDIR)/stats.o
	$(CC) $(LDFLAGS) -o $@ $^ $(FMODE_OBJ) $(LIBES) $(XDRLIB) $(MATHLIB)
