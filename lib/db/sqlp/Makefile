MODULE_TOPDIR = ../../..

LIB_NAME = $(SQLP_LIBNAME)

LIB_OBJS = \
	alloc.o \
	print.o \
	sql.o \
	lex.yy.o \
	y.tab.o

EXTRA_CLEAN_FILES = lex.yy.c y.tab.c y.output y.tab.h
EXTRA_HEADERS = y.tab.h

include $(MODULE_TOPDIR)/include/Make/Lib.make

default: lib
	$(MKDIR) $(GISBASE)/docs/html
	$(INSTALL_DATA) description.html $(GISBASE)/docs/html/sql.html

y.tab.c y.tab.h: yac.y
	$(YACC) -d -v yac.y

lex.yy.c: lex.l
	$(LEX) lex.l

$(OBJDIR)/lex.yy.o: lex.yy.c y.tab.h

