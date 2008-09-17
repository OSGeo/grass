
MODULE_TOPDIR = ../../../..

include $(MODULE_TOPDIR)/include/Make/Other.make

LIBES = $(SQLPLIB) $(DBMILIB) $(GISLIB) $(MATHLIB) $(ZLIBLIBPATH) $(ZLIB)

EXTRA_CLEAN_FILES = sqlptest

default: sqlptest


sqlptest: sqlptest.c 
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<  $(LIBES)

