MODULE_TOPDIR = ../../..

LIB = SQLP
PGM = sql

include $(MODULE_TOPDIR)/include/Make/Lib.make

default: lib

$(OBJDIR)/sqlp.yy.o: sqlp.tab.h

.SECONDARY: sqlp.tab.c sqlp.tab.h sqlp.output

.SECONDARY: sqlp.yy.c
