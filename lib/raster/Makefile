MODULE_TOPDIR = ../..

GDAL_LINK = $(USE_GDAL)
GDAL_DYNAMIC = 1

LIB_NAME = $(RASTR_LIBNAME)
EXTRA_LIBS = $(XDRLIB) $(SOCKLIB) $(DATETIMELIB) $(PTHREADLIBPATH) \
	$(PTHREADLIB) $(INTLLIB) $(MATHLIB) $(GISLIB)
EXTRA_INC = $(ZLIBINCPATH) $(PTHREADINCPATH)

include $(MODULE_TOPDIR)/include/Make/Vars.make
include $(MODULE_TOPDIR)/include/Make/Lib.make
include $(MODULE_TOPDIR)/include/Make/Doxygen.make

#compile if LFS Large File Support present:
ifneq ($(USE_LARGEFILES),)
	EXTRA_CFLAGS = -D_FILE_OFFSET_BITS=64
endif

ifneq ($(GDAL_LINK),)
EXTRA_CFLAGS += -DGDAL_LINK=1
EXTRA_INC += $(PROJINC) $(GDALCFLAGS)

ifneq ($(GDAL_DYNAMIC),)
EXTRA_CFLAGS += -DGDAL_DYNAMIC=1
ifneq ($(MINGW),)
EXTRA_LIBS += -lkernel32
else
EXTRA_LIBS += $(DLLIB)
endif
else
EXTRA_LIBS += $(GDALLIBS)
endif

endif

default: lib

DOXNAME = rasterlib

$(OBJDIR)/auto_mask.o: G.h
$(OBJDIR)/closecell.o: G.h
$(OBJDIR)/format.o: G.h
$(OBJDIR)/get_row.o: G.h
$(OBJDIR)/get_window.o: G.h
$(OBJDIR)/maskfd.o: G.h
$(OBJDIR)/opencell.o: G.h
$(OBJDIR)/put_row.o: G.h
