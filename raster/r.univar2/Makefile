
MODULE_TOPDIR = ../..

LIBES = $(G3DLIB) $(GISLIB)
DEPENDENCIES = $(G3DDEP) $(GISDEP)

#needed for htmlmulti
PROGRAMS = r.univar r3.univar

include $(MODULE_TOPDIR)/include/Make/Multi.make

R3UNIVAR = $(BIN)/r3.univar$(EXE)
RUNIVAR = $(BIN)/r.univar$(EXE)

default: $(R3UNIVAR) $(RUNIVAR)
	$(MAKE) htmlmulti

$(RUNIVAR): $(OBJDIR)/r.univar_main.o $(OBJDIR)/sort.o $(OBJDIR)/stats.o
	$(CC) $(LDFLAGS) -o $@ $^ $(FMODE_OBJ) $(LIBES) $(XDRLIB) $(MATHLIB)

$(R3UNIVAR): $(OBJDIR)/r3.univar_main.o $(OBJDIR)/sort.o $(OBJDIR)/stats.o
	$(CC) $(LDFLAGS) -o $@ $^ $(FMODE_OBJ) $(LIBES) $(XDRLIB) $(MATHLIB)

