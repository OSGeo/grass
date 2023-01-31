MODULE_TOPDIR = ../..

PGM=r.in.pdal

LIBES = $(RASTERLIB) $(SEGMENTLIB) $(GPROJLIB) $(VECTORLIB) $(DBMILIB) $(GISLIB) $(MATHLIB) $(PDALLIBS) $(GMATHLIB)
DEPENDENCIES = $(GPROJDEP) $(VECTORDEP) $(DBMIDEP) $(RASTERDEP) $(GISDEP) $(SEGMENTDEP) $(GMATHDEP)

EXTRA_INC = $(VECT_INC) $(PROJINC) $(PDALINC)
EXTRA_CFLAGS = $(VECT_CFLAGS)

include $(MODULE_TOPDIR)/include/Make/Module.make

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
=======
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
$(OBJDIR)/main.o : EXTRA_CFLAGS += $(PDALCPPFLAGS)

$(OBJDIR)/info.o : EXTRA_CFLAGS += $(PDALCPPFLAGS)

$(OBJDIR)/grasslidarfilter.o : EXTRA_CFLAGS += $(PDALCPPFLAGS)

<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 9d4a079d2e (libcairodriver: enable Cairo with and without Fontconfig (#1697))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
LINK = $(CXX)

ifneq ($(strip $(CXX)),)
ifneq ($(USE_PDAL),)
default: cmd
endif
endif
