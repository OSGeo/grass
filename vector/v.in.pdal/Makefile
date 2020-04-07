MODULE_TOPDIR = ../..

PGM=v.in.pdal

LIBES = $(GPROJLIB) $(VECTORLIB) $(DBMILIB) $(GISLIB) $(MATHLIB) $(PDALLIBS)
DEPENDENCIES = $(GPROJDEP) $(VECTORDEP) $(DBMIDEP) $(GISDEP)

EXTRA_INC = $(VECT_INC) $(PROJINC) $(PDALINC)
EXTRA_CFLAGS = $(VECT_CFLAGS)

include $(MODULE_TOPDIR)/include/Make/Module.make

$(OBJDIR)/main.o : EXTRA_CFLAGS += $(PDALCPPFLAGS)

LINK = $(CXX)

ifneq ($(strip $(CXX)),)
ifneq ($(USE_PDAL),)
default: cmd
endif
endif
