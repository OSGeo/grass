MODULE_TOPDIR = ../..

LIB = GPROJ

include $(MODULE_TOPDIR)/include/Make/Vars.make

MOD_OBJS := $(filter-out ftol.o,$(AUTO_OBJS))

include $(MODULE_TOPDIR)/include/Make/Lib.make 
include $(MODULE_TOPDIR)/include/Make/Doxygen.make

EXTRA_CFLAGS = $(ZLIBINCPATH) $(GDALCFLAGS)
EXTRA_INC = $(PROJINC)

PROJ_DIR = $(ETC)/proj

PROJ_TABLES := desc.table parms.table units.table
PROJ_DSTFILES := $(patsubst %,$(PROJ_DIR)/%,$(PROJ_TABLES))

default: lib $(FTOL_OBJ)
	$(MAKE) $(PROJ_DSTFILES)

ifdef MINGW
$(FTOL_OBJ): $(OBJDIR)/ftol.o
	$(INSTALL) $< $@
endif

$(PROJ_DSTFILES): $(PROJ_DIR)/%: % | $(PROJ_DIR)
	$(INSTALL_DATA) $< $@

$(PROJ_DIR):
	$(MKDIR) $@

#doxygen:
DOXNAME=proj
