MODULE_TOPDIR = ../../..

DEPENDENCIES = $(GISDEP)

include $(MODULE_TOPDIR)/include/Make/Platform.make
include $(MODULE_TOPDIR)/include/Make/Grass.make
include $(MODULE_TOPDIR)/include/Make/Rules.make

DRIVER=$(DBDRIVERDIR)/pg$(EXE)

LIBES = $(DBMIDRIVERLIB) $(DBMIBASELIB) $(DBMIEXTRALIB) $(SQLPLIB) $(DBSTUBSLIB) \
	$(GISLIB) $(DATETIMELIB) $(PQLIB)

EXTRA_CFLAGS = $(PQINCPATH)
EXTRA_INC = -I$(MODULE_TOPDIR)/lib/db/dbmi_driver

OBJS = $(subst .c,.o,$(wildcard *.c))

ARCH_OBJS := $(foreach obj,$(OBJS),$(OBJDIR)/$(obj))

DRVDOC=$(GISBASE)/docs/html/grass-pg.html

default: $(DRIVER) $(DRVDOC)

$(DRIVER): $(ARCH_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(ARCH_OBJS) $(FMODE_OBJ) $(PQLIBPATH) $(LIBES) \
		$(MATHLIB) $(XDRLIB)

$(DRVDOC): grass-pg.html
	$(INSTALL_DATA) grass-pg.html $(DRVDOC)
