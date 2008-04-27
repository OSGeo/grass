MODULE_TOPDIR = ../..

LIB_NAME = $(GIS_LIBNAME)
EXTRA_LIBS = $(XDRLIB) $(DATETIMELIB) $(INTLLIB)
DATASRC = ellipse.table datum.table datumtransform.table FIPS.code state27 state83 projections gui.tcl

include $(MODULE_TOPDIR)/include/Make/Lib.make
include $(MODULE_TOPDIR)/include/Make/Doxygen.make

DATAFILES := $(patsubst %,$(ETC)/%,$(DATASRC))
COLORSRC := $(filter-out colors/CVS,$(wildcard colors/[abcdefghijklmnopqrstuvwxyz]*))
COLORFILES := $(patsubst colors/%,$(ETC)/colors/%,$(COLORSRC))

#compile if LFS Large File Support present:
ifneq ($(USE_LARGEFILES),)
	EXTRA_CFLAGS = -D_FILE_OFFSET_BITS=64
endif

default: lib $(FMODE_OBJ) $(DATAFILES) $(COLORFILES)

$(FMODE_OBJ): fmode.dat
	@test -d $(OBJDIR) || mkdir $(OBJDIR)
	cat fmode.dat > $(OBJDIR)/fmode.c
	$(CC) $(CFLAGS) $(INC) -o $(FMODE_OBJ) -c $(OBJDIR)/fmode.c

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

#doxygen:
DOXNAME=gis

$(OBJDIR)/area_ellipse.o: pi.h
$(OBJDIR)/area_sphere.o: pi.h
$(OBJDIR)/auto_mask.o: G.h
$(OBJDIR)/closecell.o: G.h
$(OBJDIR)/format.o: G.h
$(OBJDIR)/geodesic.o: pi.h
$(OBJDIR)/geodist.o: pi.h
$(OBJDIR)/get_row.o: G.h
$(OBJDIR)/get_window.o: G.h
$(OBJDIR)/gisinit.o: G.h
$(OBJDIR)/maskfd.o: G.h
$(OBJDIR)/opencell.o: G.h
$(OBJDIR)/put_row.o: G.h
$(OBJDIR)/radii.o: pi.h
$(OBJDIR)/rhumbline.o: pi.h
$(OBJDIR)/set_window.o: G.h
$(OBJDIR)/window_map.o: G.h
