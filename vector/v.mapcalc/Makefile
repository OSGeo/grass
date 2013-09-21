MODULE_TOPDIR = ../..

PGM = v.mapcalc

LIBES = $(GISLIB) $(DLLIB)
DEPENDENCIES = $(GISDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

default: cmd

yylex.c: v.mapcalc.tab.h

.INTERMEDIATE: v.mapcalc.tab.c v.mapcalc.tab.h v.mapcalc.output
