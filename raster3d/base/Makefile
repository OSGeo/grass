
MODULE_TOPDIR = ../..

LIBES = $(G3DLIB) $(GISLIB)
DEPENDENCIES = $(G3DDEP) $(GISDEP)

PROGRAMS = r3.info r3.mask r3.null r3.timestamp

include $(MODULE_TOPDIR)/include/Make/Multi.make

default: multi

$(BIN)/r3.%$(EXE): $(OBJDIR)/r3.%.main.o $(OBJDIR)/mask_functions.o
	$(CC) $(LDFLAGS) -o $@ $^ $(FMODE_OBJ) $(LIBES) $(XDRLIB) $(MATHLIB)

$(BIN)/r3.mask$(EXE): $(OBJDIR)/mask_functions.o

$(BIN)/r3.null$(EXE): $(OBJDIR)/mask_functions.o
