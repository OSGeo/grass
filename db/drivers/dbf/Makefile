
MODULE_TOPDIR = ../../..

include $(MODULE_TOPDIR)/include/Make/Platform.make
include $(MODULE_TOPDIR)/include/Make/Grass.make
include $(MODULE_TOPDIR)/include/Make/Rules.make

DRIVER=$(DBDRIVERDIR)/dbf$(EXE)
DEPENDENCIES = $(GISDEP)
EXTRA_INC = -I$(MODULE_TOPDIR)/lib/db/dbmi_driver

LIBES = $(DBMIDRIVERLIB) $(DBMIBASELIB) $(DBMIEXTRALIB) $(SQLPLIB) $(SHAPELIB) $(DBSTUBSLIB) $(GISLIB) $(DATETIMELIB)

DRVDOC=$(GISBASE)/docs/html/grass-dbf.html

OBJS 	=  \
	column.o \
	cursor.o \
	db.o \
	dbfexe.o \
	describe.o \
	driver.o \
	error.o \
	execute.o \
	fetch.o \
	listtab.o \
	main.o \
	select.o \
	str.o \
	table.o \
	create_table.o

ARCH_OBJS := $(foreach obj,$(OBJS),$(OBJDIR)/$(obj))

default: $(DRIVER) $(DRVDOC)

$(DRIVER): $(ARCH_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(ARCH_OBJS) $(FMODE_OBJ) $(LIBES) $(MATHLIB) $(XDRLIB)

$(DRVDOC): grass-dbf.html
	$(INSTALL_DATA) grass-dbf.html $(DRVDOC)
