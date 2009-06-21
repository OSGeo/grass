MODULE_TOPDIR = ../..

PGM = i.latlong

LIBES =  $(GPROJLIB) $(GISLIB) $(RASTRLIB)
DEPENDENCIES = $(GPROJDEP) $(GISDEP) $(RASTRDEP)
EXTRA_INC = $(PROJINC)

include $(MODULE_TOPDIR)/include/Make/Module.make

ifneq ($(USE_LARGEFILES),)
	EXTRA_CFLAGS = -D_FILE_OFFSET_BITS=64
endif

default: cmd
