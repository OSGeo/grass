MODULE_TOPDIR = ../../

PGM = r.viewshed

LIBES = $(RASTERLIB) $(GISLIB) $(IOSTREAMLIB) $(MATHLIB)
DEPENDENCIES = $(RASTERDEP) $(GISDEP) $(IOSTREAMDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

EXTRA_CFLAGS = -DUSER=\"$(USER)\" -Wno-sign-compare

LINK = $(CXX)

ifneq ($(strip $(CXX)),)
default: cmd
endif

