MODULE_TOPDIR = ../..

PGM2 = r.mapcalc
PGM3 = r3.mapcalc

EXTRA_HEADERS = y.tab.h
EXTRA_CLEAN_FILES = y.tab.c y.tab.h lex.yy.c y.output lex.backup

include $(MODULE_TOPDIR)/include/Make/Module.make

COMMON = \
	$(OBJDIR)/y.tab.o \
	$(OBJDIR)/lex.yy.o \
	\
	$(OBJDIR)/column_shift.o \
	$(OBJDIR)/evaluate.o \
	$(OBJDIR)/expression.o \
	$(OBJDIR)/function.o \
	$(OBJDIR)/check.o \
	$(OBJDIR)/main.o \
	\
	$(OBJDIR)/xabs.o \
	$(OBJDIR)/xadd.o \
	$(OBJDIR)/xand.o \
	$(OBJDIR)/xand2.o \
	$(OBJDIR)/xacos.o \
	$(OBJDIR)/xasin.o \
	$(OBJDIR)/xatan.o \
	$(OBJDIR)/xbitand.o \
	$(OBJDIR)/xbitnot.o \
	$(OBJDIR)/xbitor.o \
	$(OBJDIR)/xbitxor.o \
	$(OBJDIR)/xcos.o \
	$(OBJDIR)/xdiv.o \
	$(OBJDIR)/xdouble.o \
	$(OBJDIR)/xeq.o \
	$(OBJDIR)/xeval.o \
	$(OBJDIR)/xexp.o \
	$(OBJDIR)/xfloat.o \
	$(OBJDIR)/xge.o \
	$(OBJDIR)/xgraph.o \
	$(OBJDIR)/xgt.o \
	$(OBJDIR)/xif.o \
	$(OBJDIR)/xint.o \
	$(OBJDIR)/xisnull.o \
	$(OBJDIR)/xle.o \
	$(OBJDIR)/xlog.o \
	$(OBJDIR)/xlt.o \
	$(OBJDIR)/xmax.o \
	$(OBJDIR)/xmedian.o \
	$(OBJDIR)/xmin.o \
	$(OBJDIR)/xmod.o \
	$(OBJDIR)/xmode.o \
	$(OBJDIR)/xmul.o \
	$(OBJDIR)/xne.o \
	$(OBJDIR)/xneg.o \
	$(OBJDIR)/xnot.o \
	$(OBJDIR)/xnull.o \
	$(OBJDIR)/xor.o \
	$(OBJDIR)/xor2.o \
	$(OBJDIR)/xpow.o \
	$(OBJDIR)/xrand.o \
	$(OBJDIR)/xround.o \
	$(OBJDIR)/xrowcol.o \
	$(OBJDIR)/xshiftl.o \
	$(OBJDIR)/xshiftr.o \
	$(OBJDIR)/xshiftru.o \
	$(OBJDIR)/xsin.o \
	$(OBJDIR)/xsqrt.o \
	$(OBJDIR)/xsub.o \
	$(OBJDIR)/xtan.o

LIST2 =	$(COMMON) \
	$(OBJDIR)/map.o \
	$(OBJDIR)/xcoor.o \
	$(OBJDIR)/xres.o

LIST3 =	$(COMMON) \
	$(OBJDIR)/map3.o \
	$(OBJDIR)/xcoor3.o \
	$(OBJDIR)/xres3.o

EXTRA_CFLAGS = $(READLINEINCPATH)
LIBES2 = $(GISLIB) $(BTREELIB) $(ROWIOLIB) $(READLINELIBPATH) $(READLINELIB) $(HISTORYLIB)
LIBES3 = $(G3DLIB) $(GISLIB) $(BTREELIB) $(READLINELIBPATH) $(READLINELIB) $(HISTORYLIB)

default: $(BIN)/$(PGM2)$(EXE) $(BIN)/$(PGM3)$(EXE) html

$(BIN)/$(PGM2)$(EXE): $(LIST2) $(DEPLIBS2)
	$(CC) $(LDFLAGS) -o $@ $(LIST2) $(FMODE_OBJ) $(LIBES2) $(MATHLIB) $(XDRLIB)

$(BIN)/$(PGM3)$(EXE): $(LIST3) $(DEPLIBS3)
	$(CC) $(LDFLAGS) -o $@ $(LIST3) $(FMODE_OBJ) $(LIBES3) $(MATHLIB) $(XDRLIB)

y.tab.c y.tab.h: mapcalc.y
	$(YACC) -b y -d mapcalc.y

lex.yy.c: mapcalc.l y.tab.h
	$(LEX) -t mapcalc.l > $@

html:
	$(MKDIR) $(GISBASE)/docs/html
	$(INSTALL_DATA) $(PGM2).html $(GISBASE)/docs/html/
	$(INSTALL_DATA) $(PGM3).html $(GISBASE)/docs/html/
