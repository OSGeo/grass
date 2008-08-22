MODULE_TOPDIR = ../..

PGM = r.terraflow

LIBES = $(GISLIB) $(IOSTREAMLIB)
DEPENDENCIES = $(GISDEP) $(IOSTREAMDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

EXTRA_CFLAGS = -DUSER=\"$(USER)\" -DNODATA_FIX -DELEV_FLOAT -Wno-sign-compare
ifneq ($(USE_LARGEFILES),)
	EXTRA_CFLAGS += -D_FILE_OFFSET_BITS=64
endif

LINK = $(CXX)

default: cmd
