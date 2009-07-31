MODULE_TOPDIR = ../..

LIB_NAME = $(GIS_LIBNAME)

EXTRA_LIBS = $(DATETIMELIB) $(PTHREADLIBPATH) $(PTHREADLIB) \
	$(INTLLIB) $(MATHLIB) $(ZLIBLIBPATH) $(ZLIB) $(GDALLIBS)
EXTRA_INC = $(ZLIBINCPATH) $(PTHREADINCPATH)

DATASRC = ellipse.table datum.table datumtransform.table FIPS.code state27 state83 projections

include $(MODULE_TOPDIR)/include/Make/Vars.make

MOD_OBJS := $(filter-out fmode.o,$(AUTO_OBJS))

include $(MODULE_TOPDIR)/include/Make/Lib.make
include $(MODULE_TOPDIR)/include/Make/Doxygen.make

DATAFILES := $(patsubst %,$(ETC)/%,$(DATASRC))

COLORSRC := $(filter-out colors/CVS,$(wildcard colors/[abcdefghijklmnopqrstuvwxyz]*))
COLORFILES := $(patsubst colors/%,$(ETC)/colors/%,$(COLORSRC))

#compile if LFS Large File Support present:
ifneq ($(USE_LARGEFILES),)
	EXTRA_CFLAGS = -D_FILE_OFFSET_BITS=64
endif

default: lib $(FMODE_OBJ) $(DATAFILES) $(COLORFILES) $(ETC)/colors.desc $(ETC)/element_list

ifdef MINGW
$(FMODE_OBJ): $(OBJDIR)/fmode.o
	$(INSTALL) $< $@
endif

$(ETC)/colors:
	$(MKDIR) $@

ifneq ($(BROKEN_MAKE),)
$(ETC)/colors/%: colors/% $(ETC)/colors
else
$(ETC)/colors/%: colors/% | $(ETC)/colors
endif
	$(INSTALL_DATA) $< $@

$(ETC)/%: %
	$(INSTALL_DATA) $< $@

DOXNAME = gis

$(OBJDIR)/area_ellipse.o: pi.h
$(OBJDIR)/area_sphere.o: pi.h
$(OBJDIR)/geodesic.o: pi.h
$(OBJDIR)/geodist.o: pi.h
$(OBJDIR)/get_window.o: G.h
$(OBJDIR)/gisinit.o: G.h
$(OBJDIR)/radii.o: pi.h
$(OBJDIR)/rhumbline.o: pi.h
$(OBJDIR)/set_window.o: G.h
