MODULE_TOPDIR = ../..

LIB_NAME = $(GPROJ_LIBNAME)

include $(MODULE_TOPDIR)/include/Make/Lib.make 
include $(MODULE_TOPDIR)/include/Make/Doxygen.make

EXTRA_LIBS=$(GISLIB) $(PROJLIB) $(GDALLIBS)
EXTRA_CFLAGS = $(GETHOSTNAME) $(ZLIBINCPATH) $(PICFLAGS) $(GDALCFLAGS) $(DEFS)
EXTRA_INC = $(PROJINC)

NAD_TABLES := $(wildcard *.lla)

NT_TABLES = ntv1_can.dat nzgd2kgrid0005.gsb

CSV_FILES := $(wildcard *.csv)

NAD_DIR = $(GISBASE)/etc/nad
NAD2BIN = nad2bin
CSV_DIR = $(GISBASE)/etc/ogr_csv

default: lib $(NAD_DIR) $(CSV_DIR)

$(NAD_DIR): $(NAD_TABLES) $(NT_TABLES)
	@ if [ ! -d $@ ] ; then $(MKDIR) $@ ; fi
	for i in $(NAD_TABLES) ; do $(NAD2BIN) < $$i $@/`echo $$i | sed 's/.lla//'`; done
	$(INSTALL_DATA) $(NT_TABLES) $@
	@ touch $@

$(CSV_DIR): $(CSV_FILES)
	@ if [ ! -d $@ ] ; then $(MKDIR) $@ ; fi
	$(INSTALL_DATA) $(CSV_FILES) $@
	@ touch $@

# Note for future improvement:
# nad2bin should be compiled from source if we are cross compiling and then the
# above NAD_TABLES files created during installation rather than compilation


#doxygen:
DOXNAME=proj
