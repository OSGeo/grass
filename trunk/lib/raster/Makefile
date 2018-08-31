MODULE_TOPDIR = ../..

LIB = RASTER
EXTRA_INC = 

include $(MODULE_TOPDIR)/include/Make/Vars.make
include $(MODULE_TOPDIR)/include/Make/Lib.make
include $(MODULE_TOPDIR)/include/Make/Doxygen.make

ifneq ($(GDAL_LINK),)
EXTRA_CFLAGS += -DGDAL_LINK=1
EXTRA_INC += $(PROJINC) $(GDALCFLAGS)

ifneq ($(GDAL_DYNAMIC),)
EXTRA_CFLAGS += -DGDAL_DYNAMIC=1
endif

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
