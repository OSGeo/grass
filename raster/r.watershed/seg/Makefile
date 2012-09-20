MODULE_TOPDIR = ../../..

PGM = r.watershed/seg
DIR = $(ETC)/r.watershed

LIBES = $(SEGMENTLIB) $(RASTERLIB) $(GISLIB) $(MATHLIB)
DEPENDENCIES = $(SEGMENTDEP) $(RASTERDEP) $(GISDEP)

include $(MODULE_TOPDIR)/include/Make/Etc.make
include $(MODULE_TOPDIR)/include/Make/NoHtml.make

default: $(DIR) etc

$(DIR):
	$(MKDIR) $@
