
MODULE_TOPDIR = ../..

LIBES = $(G3DLIB) $(GISLIB)
DEPENDENCIES = $(G3DDEP) $(GISDEP)

#needed for htmlmulti
PROGRAMS = r3.info r3.mask r3.null r3.timestamp

include $(MODULE_TOPDIR)/include/Make/Multi.make

R3INFO = $(BIN)/r3.info$(EXE)
R3MASK = $(BIN)/r3.mask$(EXE)
R3NULL = $(BIN)/r3.null$(EXE)
R3TIMESTAMP = $(BIN)/r3.timestamp$(EXE)

default: $(R3INFO) $(R3MASK) $(R3NULL) $(R3TIMESTAMP)
	$(MAKE) htmlmulti

$(R3MASK): $(OBJDIR)/r3.mask.main.o $(OBJDIR)/mask_functions.o
	$(CC) $(LDFLAGS) -o $@ $^ $(FMODE_OBJ) $(LIBES) $(XDRLIB) $(MATHLIB)

$(R3NULL): $(OBJDIR)/r3.null.main.o $(OBJDIR)/mask_functions.o
	$(CC) $(LDFLAGS) -o $@ $^ $(FMODE_OBJ) $(LIBES) $(XDRLIB) $(MATHLIB)

$(R3TIMESTAMP): $(OBJDIR)/r3.timestamp.main.o 
	$(CC) $(LDFLAGS) -o $@ $^ $(FMODE_OBJ) $(LIBES) $(XDRLIB) $(MATHLIB)

$(R3INFO): $(OBJDIR)/r3.info.main.o 
	$(CC) $(LDFLAGS) -o $@ $^ $(FMODE_OBJ) $(LIBES) $(XDRLIB) $(MATHLIB)

