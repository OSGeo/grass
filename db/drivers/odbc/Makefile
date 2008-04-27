MODULE_TOPDIR = ../../..

include $(MODULE_TOPDIR)/include/Make/Platform.make
include $(MODULE_TOPDIR)/include/Make/Grass.make
include $(MODULE_TOPDIR)/include/Make/Rules.make

DRIVER=$(DBDRIVERDIR)/odbc$(EXE)
EXTRA_INC = $(ODBCINC) -I$(MODULE_TOPDIR)/lib/db/dbmi_driver
DEPENDENCIES = $(GISDEP)

LIBES = $(DBMIDRIVERLIB) $(DBMIBASELIB) $(DBMIEXTRALIB) $(DBSTUBSLIB) $(GISLIB) $(DATETIMELIB)

OBJS 	=  \
	connect.o \
	cursor.o \
	db.o \
	describe.o \
	driver.o \
	error.o \
	execute.o \
	fetch.o \
	listdb.o \
	listtab.o \
	main.o \
	select.o \
	table.o \
	create_table.o

ARCH_OBJS := $(foreach obj,$(OBJS),$(OBJDIR)/$(obj))

DRVDOC=$(GISBASE)/docs/html/grass-odbc.html

default: $(DRIVER) $(DRVDOC)

$(DRIVER): $(ARCH_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(ARCH_OBJS) $(FMODE_OBJ) $(LIBES) $(ODBCLIB) $(MATHLIB) $(XDRLIB) $(ICONVLIB)

$(DRVDOC): grass-odbc.html
	$(INSTALL_DATA) grass-odbc.html $(DRVDOC)
