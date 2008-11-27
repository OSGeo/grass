
MODULE_TOPDIR = ../..

PGM=nviz

include $(MODULE_TOPDIR)/include/Make/Other.make

VER=2.2
NV_VER=nviz$(VER)
INSTALL_DIR=$(ETC)/$(NV_VER)
CLEAN_SUBDIRS = src

SCRIPTS := \
	scripts/script_get_line \
	scripts/script_play \
	scripts/script_tools \
	scripts/script_file_tools \
	scripts/nviz_params \
	scripts/$(NV_VER)_script \
	$(wildcard scripts/*.tcl) \
	$(wildcard scripts/*Index)
SCRIPT_DST := $(patsubst scripts/%,$(INSTALL_DIR)/scripts/%,$(SCRIPTS))

BITMAPS := $(filter-out flythrough,$(wildcard bitmaps/*)) $(wildcard bitmaps/flythrough/*.gif)
BITMAP_DST := $(patsubst bitmaps/%,$(INSTALL_DIR)/bitmaps/%,$(BITMAPS))

ifneq ($(USE_OPENGL),)
ifneq ($(strip $(TCLTKLIBS)),)
default:
	$(MAKE) -C src
	$(MAKE) install
	$(MAKE) -C html
	$(MAKE) html
endif
endif

# Install the scripts in the appropriate directory
install:
	-if [ ! -d $(INSTALL_DIR) ] ; then $(MKDIR) $(INSTALL_DIR) ; fi
	$(MAKE) $(INSTALL_DIR)/$(PGM)$(EXE)
	$(MAKE) $(BIN)/$(PGM)
ifneq ($(strip $(MINGW)),)
	$(MAKE) $(BIN)/nviz.bat
endif
	-if [ ! -d $(INSTALL_DIR)/scripts ] ; then $(MKDIR) $(INSTALL_DIR)/scripts ; fi
	$(MAKE) $(SCRIPT_DST)
	-if [ ! -d $(INSTALL_DIR)/bitmaps ] ; then $(MKDIR) $(INSTALL_DIR)/bitmaps ; fi
	-if [ ! -d $(INSTALL_DIR)/bitmaps/flythrough ] ; then $(MKDIR) $(INSTALL_DIR)/bitmaps/flythrough ; fi
	$(MAKE) $(BITMAP_DST)

$(INSTALL_DIR)/$(PGM)$(EXE): src/nvwish$(EXE)
	$(INSTALL) $< $@

$(BIN)/$(PGM): scripts/$(PGM)
	$(INSTALL) $< $@

$(BIN)/nviz.bat: scripts/nviz.bat
	$(INSTALL) $< $@

$(INSTALL_DIR)/scripts/%: scripts/%
	$(INSTALL) $< $@

$(INSTALL_DIR)/bitmaps/%: bitmaps/%
	$(INSTALL_DATA) $< $@

.PHONY: install
