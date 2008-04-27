MODULE_TOPDIR = ../../..

DEPENDENCIES = $(GISDEP)

include $(MODULE_TOPDIR)/include/Make/Platform.make
include $(MODULE_TOPDIR)/include/Make/Grass.make
include $(MODULE_TOPDIR)/include/Make/Rules.make

DRIVER=$(DBDRIVERDIR)/ogr$(EXE)

LIBES = $(DBMIDRIVERLIB) $(DBMIBASELIB) $(DBMIEXTRALIB) $(SQLPLIB) $(DBSTUBSLIB) \
	$(GISLIB) $(DATETIMELIB) $(GDALLIBS)
EXTRA_CFLAGS = $(GDALCFLAGS) $(TCLINCDIR) $(TKINCDIR)
EXTRA_INC = -I$(MODULE_TOPDIR)/lib/db/dbmi_driver

DRVDOC=$(GISBASE)/docs/html/grass-ogr.html

OBJS = $(subst .c,.o,$(wildcard *.c))

ARCH_OBJS := $(foreach obj,$(OBJS),$(OBJDIR)/$(obj))

default: $(DRIVER) $(DRVDOC)

$(DRIVER): $(ARCH_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(ARCH_OBJS) $(PQLIBPATH) $(FMODE_OBJ) $(LIBES) $(TCLTKLIBPATH) $(TCLTKLIBS) \
		$(MATHLIB) $(XDRLIB)

$(DRVDOC): grass-ogr.html
	$(INSTALL_DATA) grass-ogr.html $(DRVDOC)
