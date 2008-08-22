
MODULE_TOPDIR = ../..

LIBES = $(DBMILIB) $(GISLIB) $(DATETIMELIB)
DEPENDENCIES = $(GISDEP) $(DBMIDEP)

#not used: db.createdb db.dropdb db.databases db.droptable
PROGRAMS = db.columns db.copy db.describe db.drivers db.execute db.select db.tables db.connect

db_columns_OBJS = columns.o
db_copy_OBJS = copy.o
db_describe_OBJS = describe.o printtab.o
db_drivers_OBJS = drivers.o
db_execute_OBJS = execute.o
db_select_OBJS = select.o printtab.o
db_tables_OBJS = tables.o
db_connect_OBJS = connect.o

include $(MODULE_TOPDIR)/include/Make/Multi.make

default: multi

