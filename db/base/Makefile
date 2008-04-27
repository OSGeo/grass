
MODULE_TOPDIR = ../..

LIBES = $(DBMILIB) $(GISLIB) $(DATETIMELIB)
DEPENDENCIES = $(GISDEP) $(DBMIDEP)

#not used: db.createdb db.dropdb db.databases db.droptable
PROGRAMS = db.columns db.copy db.describe db.drivers db.execute db.select db.tables db.connect

include $(MODULE_TOPDIR)/include/Make/Multi.make

default: $(patsubst %,$(BIN)/%$(EXE),$(PROGRAMS))
	$(MAKE) htmlmulti

$(BIN)/db.columns$(EXE): $(OBJDIR)/columns.o
	$(CC) $(LDFLAGS) -o $@ $^ $(FMODE_OBJ) $(LIBES) $(XDRLIB) $(MATHLIB)

$(BIN)/db.copy$(EXE): $(OBJDIR)/copy.o
	$(CC) $(LDFLAGS) -o $@ $^ $(FMODE_OBJ) $(LIBES) $(XDRLIB) $(MATHLIB)

$(BIN)/db.createdb$(EXE): $(OBJDIR)/createdb.o
	$(CC) $(LDFLAGS) -o $@ $^ $(FMODE_OBJ) $(LIBES) $(XDRLIB) $(MATHLIB)

$(BIN)/db.dropdb$(EXE): $(OBJDIR)/dropdb.o
	$(CC) $(LDFLAGS) -o $@ $^ $(FMODE_OBJ) $(LIBES) $(XDRLIB) $(MATHLIB)

$(BIN)/db.droptable$(EXE): $(OBJDIR)/droptable.o
	$(CC) $(LDFLAGS) -o $@ $^ $(FMODE_OBJ) $(LIBES) $(XDRLIB) $(MATHLIB)

$(BIN)/db.databases$(EXE): $(OBJDIR)/databases.o
	$(CC) $(LDFLAGS) -o $@ $^ $(FMODE_OBJ) $(LIBES) $(XDRLIB) $(MATHLIB)

$(BIN)/db.describe$(EXE): $(OBJDIR)/describe.o $(OBJDIR)/printtab.o
	$(CC) $(LDFLAGS) -o $@ $^ $(FMODE_OBJ) $(LIBES) $(XDRLIB) $(MATHLIB)

$(BIN)/db.drivers$(EXE): $(OBJDIR)/drivers.o
	$(CC) $(LDFLAGS) -o $@ $^ $(FMODE_OBJ) $(LIBES) $(XDRLIB) $(MATHLIB)

$(BIN)/db.execute$(EXE): $(OBJDIR)/execute.o
	$(CC) $(LDFLAGS) -o $@ $^ $(FMODE_OBJ) $(LIBES) $(XDRLIB) $(MATHLIB)

$(BIN)/db.select$(EXE): $(OBJDIR)/select.o $(OBJDIR)/printtab.o
	$(CC) $(LDFLAGS) -o $@ $^ $(FMODE_OBJ) $(LIBES) $(XDRLIB) $(MATHLIB)

$(BIN)/db.tables$(EXE): $(OBJDIR)/tables.o
	$(CC) $(LDFLAGS) -o $@ $^ $(FMODE_OBJ) $(LIBES) $(XDRLIB) $(MATHLIB)

$(BIN)/db.connect$(EXE): $(OBJDIR)/connect.o
	$(CC) $(LDFLAGS) -o $@ $^ $(FMODE_OBJ) $(LIBES) $(XDRLIB) $(MATHLIB)

