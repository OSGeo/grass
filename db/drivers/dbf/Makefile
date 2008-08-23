MODULE_TOPDIR = ../../..

DEPENDENCIES = $(GISDEP)

PGM = dbf

include $(MODULE_TOPDIR)/include/Make/Vars.make
MOD_OBJS := $(filter-out main_debug.o,$(AUTO_OBJS))
include $(MODULE_TOPDIR)/include/Make/DB.make

LIBES = $(DBMIDRIVERLIB) $(DBMIBASELIB) $(DBMIEXTRALIB) $(SQLPLIB) $(SHAPELIB) $(DBSTUBSLIB) $(GISLIB) $(DATETIMELIB)

default: dbmi
