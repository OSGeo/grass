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

PROJ_DIR = $(ETC)/proj
NAD_DIR  = $(ETC)/proj/nad

NAD_DSTFILES := $(patsubst %.lla,$(NAD_DIR)/%,$(NAD_TABLES))
NAD_BINFILES := $(patsubst %.lla,$(OBJDIR)/%,$(NAD_TABLES))
NT_DSTFILES  := $(patsubst %,$(NAD_DIR)/%,$(NT_TABLES))

NAD_SRCDIR = $(ETC)/proj/nad/src
NAD_SRCFILES := $(patsubst %.lla,$(NAD_SRCDIR)/%.lla,$(NAD_TABLES))

PROJ_TABLES := desc.table parms.table units.table
PROJ_DSTFILES := $(patsubst %,$(PROJ_DIR)/%,$(PROJ_TABLES))

default: lib $(FTOL_OBJ)
	$(MAKE) $(NAD_DSTFILES) $(PROJ_DSTFILES) $(NAD_SRCFILES)

ifdef MINGW
$(FTOL_OBJ): $(OBJDIR)/ftol.o
	$(INSTALL) $< $@
endif

$(NAD_DSTFILES): $(NAD_DIR)/%: $(OBJDIR)/% | $(NAD_DIR)
	$(INSTALL_DATA) $< $@

$(NAD_BINFILES): $(OBJDIR)/%: %.lla
	$(NAD2BIN) $@ < $< 

$(NT_DSTFILES): $(NAD_DIR)/%: % | $(NAD_DIR)
	$(INSTALL_DATA) $< $@

$(NAD_SRCFILES): $(NAD_SRCDIR)/%.lla: %.lla | $(NAD_SRCDIR)
	$(INSTALL_DATA) $< $@

$(PROJ_DSTFILES): $(PROJ_DIR)/%: % | $(PROJ_DIR)
	$(INSTALL_DATA) $< $@

$(PROJ_DIR):
	$(MKDIR) $@

$(NAD_DIR): | $(PROJ_DIR)
	$(MKDIR) $@

$(NAD_SRCDIR): | $(PROJ_DIR)
	$(MKDIR) $@

# Note for future improvement:
# To allow for cross-compilation, the above NAD_TABLES files should be
# created during installation rather than compilation

#doxygen:
DOXNAME=proj
