MODULE_TOPDIR = ../..

PGM = i.sunhours

LIBES =  $(GPROJLIB) $(GISLIB)
DEPENDENCIES = $(GPROJDEP) $(GISDEP)
EXTRA_INC = $(PROJINC)

include $(MODULE_TOPDIR)/include/Make/Module.make

ifneq ($(USE_LARGEFILES),)
	EXTRA_CFLAGS = -D_FILE_OFFSET_BITS=64
endif

default: cmd
