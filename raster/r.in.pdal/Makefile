MODULE_TOPDIR = ../..

PGM=r.in.pdal

LIBES = $(RASTERLIB) $(SEGMENTLIB) $(GPROJLIB) $(VECTORLIB) $(DBMILIB) $(GISLIB) $(MATHLIB) $(PDALLIBS) $(GMATHLIB)
DEPENDENCIES = $(GPROJDEP) $(VECTORDEP) $(DBMIDEP) $(RASTERDEP) $(GISDEP) $(SEGMENTDEP) $(GMATHDEP)

EXTRA_INC = $(VECT_INC) $(PROJINC) $(PDALINC)
EXTRA_CFLAGS = $(VECT_CFLAGS)

include $(MODULE_TOPDIR)/include/Make/Module.make

<<<<<<< HEAD
=======
$(OBJDIR)/main.o : EXTRA_CFLAGS += $(PDALCPPFLAGS)

$(OBJDIR)/info.o : EXTRA_CFLAGS += $(PDALCPPFLAGS)

$(OBJDIR)/grasslidarfilter.o : EXTRA_CFLAGS += $(PDALCPPFLAGS)

>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
LINK = $(CXX)

ifneq ($(strip $(CXX)),)
ifneq ($(USE_PDAL),)
default: cmd
endif
endif
