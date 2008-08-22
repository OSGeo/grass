
MODULE_TOPDIR = ../..

LIBES = $(DBMILIB) $(GISLIB) $(DATETIMELIB)
DEPENDENCIES = $(GISDEP) $(DBMIDEP)

#not used: db.createdb db.dropdb db.databases db.droptable
PROGRAMS = db.columns db.copy db.describe db.drivers db.execute db.select db.tables db.connect

include $(MODULE_TOPDIR)/include/Make/Multi.make

default: multi

$(BIN)/db.%$(EXE): $(OBJDIR)/%.o
	$(CC) $(LDFLAGS) -o $@ $^ $(FMODE_OBJ) $(LIBES) $(XDRLIB) $(MATHLIB)

$(BIN)/db.describe$(EXE): $(OBJDIR)/printtab.o

$(BIN)/db.select$(EXE): $(OBJDIR)/printtab.o

