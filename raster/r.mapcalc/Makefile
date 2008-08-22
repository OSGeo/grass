MODULE_TOPDIR = ../..

PGM2 = r.mapcalc
PGM3 = r3.mapcalc
PROGRAMS = $(PGM2) $(PGM3)

EXTRA_HEADERS = y.tab.h
EXTRA_CLEAN_FILES = y.tab.c y.tab.h lex.yy.c y.output lex.backup

OBJS1 := y.tab.o lex.yy.o
OBJS2 := $(filter-out $(OBJS1), $(patsubst %.c,%.o,$(wildcard *.c)))
OBJS := $(OBJS1) $(OBJS2)
r_mapcalc_OBJS  := $(filter-out map3.o xcoor3.o xres3.o, $(OBJS))
r3_mapcalc_OBJS := $(filter-out map.o xcoor.o xres.o, $(OBJS))

include $(MODULE_TOPDIR)/include/Make/Multi.make

EXTRA_CFLAGS = $(READLINEINCPATH)
LIBES2 = $(GISLIB) $(BTREELIB) $(ROWIOLIB) $(READLINELIBPATH) $(READLINELIB) $(HISTORYLIB)
LIBES3 = $(G3DLIB) $(GISLIB) $(BTREELIB) $(READLINELIBPATH) $(READLINELIB) $(HISTORYLIB)

default: multi

$(BIN)/$(PGM2)$(EXE): LIBES = $(LIBES2)
$(BIN)/$(PGM3)$(EXE): LIBES = $(LIBES3)

y.tab.c y.tab.h: mapcalc.y
	$(YACC) -b y -d mapcalc.y

lex.yy.c: mapcalc.l y.tab.h
	$(LEX) -t mapcalc.l > $@
