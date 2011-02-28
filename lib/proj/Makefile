MODULE_TOPDIR = ../..

LIB = GPROJ

include $(MODULE_TOPDIR)/include/Make/Vars.make

MOD_OBJS := $(filter-out ftol.o,$(AUTO_OBJS))

include $(MODULE_TOPDIR)/include/Make/Lib.make 
include $(MODULE_TOPDIR)/include/Make/Doxygen.make

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

NAD_SRCDIR = $(ETC)/nad/src
NAD_SRCFILES := $(patsubst %.lla,$(NAD_SRCDIR)/%.lla,$(NAD_TABLES))

PROJ_TABLES := proj-desc.table proj-parms.table proj-units.table
PROJ_DSTFILES := $(patsubst %,$(ETC)/%,$(PROJ_TABLES))

default: lib $(FTOL_OBJ)
	$(MAKE) $(NAD_DSTFILES) $(CSV_DSTFILES) $(PROJ_DSTFILES) $(NAD_SRCFILES)

ifdef MINGW
$(FTOL_OBJ): $(OBJDIR)/ftol.o
	$(INSTALL) $< $@
endif

$(NAD_DSTFILES): $(NAD_DIR)/%: %.lla | $(NAD_DIR)
	$(NAD2BIN) < $< $@

$(NT_DSTFILES): $(NAD_DIR)/%: % | $(NAD_DIR)
	$(INSTALL_DATA) $< $@

$(CSV_DSTFILES): $(CSV_DIR)/%.csv: %.csv | $(CSV_DIR)
	$(INSTALL_DATA) $< $@

$(NAD_SRCFILES): $(NAD_SRCDIR)/%.lla: %.lla | $(NAD_SRCDIR)
	$(INSTALL_DATA) $< $@

$(ETC)/proj-%.table: proj-%.table
	$(INSTALL_DATA) $< $@

$(NAD_DIR):
	$(MKDIR) $@

$(CSV_DIR):
	$(MKDIR) $@

$(NAD_SRCDIR):
	$(MKDIR) $@

# Note for future improvement:
# To allow for cross-compilation, the above NAD_TABLES files should be
# created during installation rather than compilation

#doxygen:
DOXNAME=proj
