MODULE_TOPDIR = ../../..

LIB_NAME = $(SQLP_LIBNAME)
PGM = sql

include $(MODULE_TOPDIR)/include/Make/Lib.make

default: lib

$(OBJDIR)/sqlp.yy.o: sqlp.tab.h

.INTERMEDIATE: sqlp.tab.c sqlp.tab.h sqlp.output

.INTERMEDIATE: sqlp.yy.c
