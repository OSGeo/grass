MODULE_TOPDIR = ../..

PGM = r.terraflow

LIBES = $(GISLIB) $(RASTERLIB) $(IOSTREAMLIB) $(MATHLIB)
DEPENDENCIES = $(GISDEP) $(RASTERDEP) $(IOSTREAMDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

EXTRA_CFLAGS = -DUSER=\"$(USER)\" -DNODATA_FIX -DELEV_FLOAT -Wno-sign-compare

LINK = $(CXX)

ifneq ($(strip $(CXX)),)
default: cmd
endif
