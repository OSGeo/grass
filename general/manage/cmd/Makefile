
MODULE_TOPDIR = ../../..

PROGRAMS = g.copy g.list g.remove g.rename

include $(MODULE_TOPDIR)/include/Make/Multi.make

LIBES = $(MANAGELIB) $(VECTLIB) $(DBMILIB) $(GISLIB) $(G3DLIB)
LIST = $(OBJDIR)/init.o  
DEPENDENCIES = $(GISDEP) $(VECTDEP) $(DBMIDEP) $(MANAGEDEP) $(G3DDEP)

EXTRA_INC = -I..

COPY_MOD = $(BIN)/g.copy$(EXE)
LIST_MOD = $(BIN)/g.list$(EXE)
REMOVE_MOD = $(BIN)/g.remove$(EXE)
RENAME_MOD = $(BIN)/g.rename$(EXE)

default: $(COPY_MOD) $(LIST_MOD) $(REMOVE_MOD) $(RENAME_MOD)
	$(MAKE) htmlmulti

$(COPY_MOD): $(OBJDIR)/copy.o $(LIST)
	$(CC) $(LDFLAGS) -o $@ $^ $(FMODE_OBJ) $(LIBES) $(MATHLIB) $(XDRLIB)

$(LIST_MOD): $(OBJDIR)/list.o $(LIST)
	$(CC) $(LDFLAGS) -o $@ $^ $(FMODE_OBJ) $(LIBES) $(MATHLIB) $(XDRLIB)

$(REMOVE_MOD): $(OBJDIR)/remove.o $(LIST)
	$(CC) $(LDFLAGS) -o $@ $^ $(FMODE_OBJ) $(LIBES) $(MATHLIB) $(XDRLIB)

$(RENAME_MOD): $(OBJDIR)/rename.o $(LIST)
	$(CC) $(LDFLAGS) -o $@ $^ $(FMODE_OBJ) $(LIBES) $(MATHLIB) $(XDRLIB)

