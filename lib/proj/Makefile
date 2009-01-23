MODULE_TOPDIR = ../..

LIB_NAME = $(GPROJ_LIBNAME)

include $(MODULE_TOPDIR)/include/Make/Lib.make 
include $(MODULE_TOPDIR)/include/Make/Doxygen.make

EXTRA_LIBS=$(GISLIB) $(PROJLIB) $(GDALLIBS) $(MATHLIB)
EXTRA_CFLAGS = $(ZLIBINCPATH) $(GDALCFLAGS)
EXTRA_INC = $(PROJINC)

NAD_TABLES := $(wildcard *.lla)

NT_TABLES = ntv1_can.dat nzgd2kgrid0005.gsb

CSV_FILES := $(wildcard *.csv)

NAD_DIR = $(ETC)/nad
CSV_DIR = $(ETC)/ogr_csv

NAD_DSTFILES := $(patsubst %.lla,$(NAD_DIR)/%,$(NAD_TABLES))
NT_DSTFILES  := $(patsubst %,$(NAD_DIR)/%,$(NT_TABLES))
CSV_DSTFILES := $(patsubst %,$(CSV_DIR)/%,$(CSV_FILES))

default: lib
	if [ ! -d $(NAD_DIR) ] ; then $(MKDIR) $(NAD_DIR) ; fi
	if [ ! -d $(CSV_DIR) ] ; then $(MKDIR) $(CSV_DIR) ; fi
	$(MAKE) $(NAD_DSTFILES) $(CSV_DSTFILES)

$(NAD_DIR)/%: %.lla
	$(NAD2BIN) < $< $@

$(NT_DSTFILES): $(NAD_DIR)/%: %
	$(INSTALL_DATA) $< $@

$(CSV_DIR)/%.csv: %.csv
	$(INSTALL_DATA) $< $@

# Note for future improvement:
# To allow for cross-compilation, the above NAD_TABLES files should be
# created during installation rather than compilation

#doxygen:
DOXNAME=proj
