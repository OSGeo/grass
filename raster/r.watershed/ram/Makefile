MODULE_TOPDIR = ../../..

PGM = r.watershed/ram
DIR = $(ETC)/r.watershed

LIBES = $(RASTERLIB) $(GISLIB) $(MATHLIB) $(BTREE2LIB)
DEPENDENCIES = $(RASTERDEP) $(GISDEP)

include $(MODULE_TOPDIR)/include/Make/Etc.make
include $(MODULE_TOPDIR)/include/Make/NoHtml.make

default: $(DIR) etc

$(DIR):
	$(MKDIR) $@
