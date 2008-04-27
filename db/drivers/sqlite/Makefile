MODULE_TOPDIR = ../../..

DEPENDENCIES = $(GISDEP)

include $(MODULE_TOPDIR)/include/Make/Platform.make
include $(MODULE_TOPDIR)/include/Make/Grass.make
include $(MODULE_TOPDIR)/include/Make/Rules.make

DRIVER=$(DBDRIVERDIR)/sqlite$(EXE)

LIBES = $(DBMIDRIVERLIB) $(DBMIBASELIB) $(DBMIEXTRALIB) $(DBSTUBSLIB) \
	$(GISLIB) $(DATETIMELIB) $(SQLITELIB)

EXTRA_CFLAGS = $(SQLITEINCPATH) 
EXTRA_INC = -I$(MODULE_TOPDIR)/lib/db/dbmi_driver

OBJS = $(subst .c,.o,$(wildcard *.c))

ARCH_OBJS := $(foreach obj,$(OBJS),$(OBJDIR)/$(obj))

DRVDOC=$(GISBASE)/docs/html/grass-sqlite.html

default: $(DRIVER) $(DRVDOC)

$(DRIVER): $(ARCH_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(ARCH_OBJS) $(FMODE_OBJ) $(SQLITELIBPATH) $(LIBES) \
		$(MATHLIB) $(XDRLIB)

$(DRVDOC): grass-sqlite.html
	$(INSTALL_DATA) grass-sqlite.html $(DRVDOC)
