
MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Platform.make
include $(MODULE_TOPDIR)/include/Make/Grass.make

VER=2.2
NV_VER=nviz$(VER)
PROG=nviz
INSTALL_DIR=$(GISBASE)/etc/$(NV_VER)

SCRIPTS := \
	scripts/script_get_line \
	scripts/script_play \
	scripts/script_tools \
	scripts/script_file_tools \
	scripts/nviz_params \
	scripts/$(NV_VER)_script \
	$(wildcard scripts/*.tcl) \
	$(wildcard scripts/*Index)

default:
	$(MAKE) -C src
	$(MAKE) install
	$(MAKE) -C html
	$(MAKE) docs

# Install the scripts in the appropriate directory
install:
	-$(MKDIR) $(INSTALL_DIR)
	$(INSTALL) src/nvwish$(EXE) $(INSTALL_DIR)/$(PROG)$(EXE)
	$(INSTALL) scripts/$(PROG) $(GISBASE)/bin
ifneq ($(strip $(MINGW)),)
	$(INSTALL) scripts/nviz.bat $(GISBASE)/bin
endif
	-$(MKDIR) $(INSTALL_DIR)/scripts
	for file in $(SCRIPTS) ; do $(INSTALL) $$file $(INSTALL_DIR)/scripts ; done
	-$(MKDIR) $(INSTALL_DIR)/bitmaps
	-$(MKDIR) $(INSTALL_DIR)/bitmaps/flythrough
	find bitmaps -path '*/.svn' -prune -o -type f -print | \
		while read file ; do $(INSTALL_DATA) $$file $(INSTALL_DIR)/$$file ; done

docs:
	-$(MKDIR) $(ARCH_DISTDIR)/docs/html
	$(INSTALL_DATA) description.html $(ARCH_DISTDIR)/docs/html/$(PROG).html

clean: 
	$(MAKE) -C src clean
