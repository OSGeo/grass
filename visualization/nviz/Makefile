
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
	$(MAKE) $(SCRIPT_DST)
	$(MAKE) $(BITMAP_DST)

$(INSTALL_DIR):
	$(MKDIR) $@

$(INSTALL_DIR)/scripts: | $(INSTALL_DIR)
	$(MKDIR) $@

$(INSTALL_DIR)/bitmaps: | $(INSTALL_DIR)
	$(MKDIR) $@

$(INSTALL_DIR)/bitmaps/flythrough: | $(INSTALL_DIR)/bitmaps
	$(MKDIR) $@

$(INSTALL_DIR)/$(PGM)$(EXE): src/nvwish$(EXE) | $(INSTALL_DIR)
	$(INSTALL) $< $@

$(INSTALL_DIR)/scripts/%: scripts/% | $(INSTALL_DIR)/scripts
	$(INSTALL_DATA) $< $@

$(INSTALL_DIR)/bitmaps/%: bitmaps/% | $(INSTALL_DIR)/bitmaps
	$(INSTALL_DATA) $< $@

.PHONY: install
