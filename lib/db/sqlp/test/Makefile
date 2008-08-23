
MODULE_TOPDIR = ../../../..

include $(MODULE_TOPDIR)/include/Make/Other.make

LIBES = $(SQLPLIB) $(DBMILIB) $(GISLIB) $(LEXLIB) 

EXTRA_CLEAN_FILES = sqlptest

default: sqlptest


sqlptest: sqlptest.c 
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ sqlptest.c  $(LIBES) -lm -lz 

