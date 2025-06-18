MODULE_TOPDIR = ../..

LIB = RASTER

include $(MODULE_TOPDIR)/include/Make/Vars.make
include $(MODULE_TOPDIR)/include/Make/Lib.make
include $(MODULE_TOPDIR)/include/Make/Doxygen.make

LIBES = $(OPENMP_LIBPATH) $(OPENMP_LIB)
EXTRA_INC = $(OPENMP_INCPATH)
EXTRA_CFLAGS = $(OPENMP_CFLAGS)

ifneq ($(GDAL_LINK),)
EXTRA_CFLAGS += -DGDAL_LINK=1
EXTRA_INC += $(PROJINC) $(GDALCFLAGS)
endif

default: lib

DOXNAME = raster

$(OBJDIR)/auto_mask.o: R.h
$(OBJDIR)/closecell.o: R.h
$(OBJDIR)/format.o: R.h
$(OBJDIR)/get_row.o: R.h
$(OBJDIR)/get_window.o: R.h
$(OBJDIR)/maskfd.o: R.h
$(OBJDIR)/opencell.o: R.h
$(OBJDIR)/put_row.o: R.h
$(OBJDIR)/window_map.o: R.h
