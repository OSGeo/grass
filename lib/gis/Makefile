MODULE_TOPDIR = ../..

LIB = GIS

EXTRA_INC = $(ZLIBINCPATH) $(BZIP2INCPATH) $(ZSTDINCPATH) $(PTHREADINCPATH) $(REGEXINCPATH)
EXTRA_CFLAGS = -DGRASS_VERSION_DATE=\"'$(GRASS_VERSION_DATE)'\"

PROJSRC = ellipse.table ellipse.table.solar.system datum.table \
	datumtransform.table FIPS.code state27 state83 projections

include $(MODULE_TOPDIR)/include/Make/Vars.make

MOD_OBJS := $(filter-out fmode.o,$(AUTO_OBJS))

include $(MODULE_TOPDIR)/include/Make/Lib.make
include $(MODULE_TOPDIR)/include/Make/Doxygen.make

PROJFILES := $(patsubst %,$(ETC)/proj/%,$(PROJSRC))

COLORSRC := $(filter-out colors/CVS,$(wildcard colors/[abcdefghijklmnopqrstuvwxyz]*))
COLORFILES := $(patsubst colors/%,$(ETC)/colors/%,$(COLORSRC))

default: lib $(FMODE_OBJ) $(PROJFILES) $(COLORFILES) $(ETC)/colors.desc $(ETC)/renamed_options

ifdef MINGW
$(FMODE_OBJ): $(OBJDIR)/fmode.o
	$(INSTALL) $< $@
endif

$(ETC)/colors:
	$(MKDIR) $@

$(ETC)/colors/%: colors/% | $(ETC)/colors
	$(INSTALL_DATA) $< $@

$(ETC)/proj:
	$(MKDIR) $@

$(ETC)/proj/%: % | $(ETC)/proj
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
