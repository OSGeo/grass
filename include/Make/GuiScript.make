
# common dependencies and rules for building GUI module

include $(MODULE_TOPDIR)/include/Make/Vars.make

include $(MODULE_TOPDIR)/include/Make/Rules.make
include $(MODULE_TOPDIR)/include/Make/ScriptRules.make
include $(MODULE_TOPDIR)/include/Make/HtmlRules.make

MODULES  := $(patsubst g.gui.%.py,%,$(wildcard g.gui.*.py))
CMDHTML  := $(patsubst %,$(HTMLDIR)/g.gui.%.html,$(MODULES))
GUIHTML  := $(patsubst %,$(HTMLDIR)/wxGUI.%.html,$(MODULES))
CMDMAN := $(patsubst %,$(MDDIR)/source/g.gui.%.md,$(MODULES))
GUIMAN  := $(patsubst %,$(MDDIR)/source/wxGUI.%.md,$(MODULES))
ifdef MINGW
SCRIPTEXT = .py
BATFILES  := $(patsubst %,$(BIN)/g.gui.%.bat,$(MODULES))
else
SCRIPTEXT =
BATFILES =
endif
PYFILES  := $(patsubst %,$(SCRIPTDIR)/g.gui.%$(SCRIPTEXT),$(MODULES))

guiscript: $(IMGDST) $(IMGDST_MD) $(PYFILES) $(BATFILES)
# we cannot use cross-compiled g.parser for generating html files
ifndef CROSS_COMPILING
	$(MAKE) $(CMDHTML)
	-rm -f g.gui.*.tmp.html
	$(MAKE) $(GUIHTML)
	$(MAKE) $(CMDMAN)
	$(MAKE) $(GUIMAN)
endif

$(HTMLDIR)/g.gui.%.html: g.gui.%.html g.gui.%.tmp.html | $(HTMLDIR)
	VERSION_NUMBER=$(GRASS_VERSION_NUMBER) VERSION_DATE=$(GRASS_VERSION_DATE) MODULE_TOPDIR=$(MODULE_TOPDIR) \
        $(PYTHON) $(GISBASE)/utils/mkhtml.py g.gui.$* $(GRASS_VERSION_DATE) > $@

$(MDDIR)/source/g.gui.%.md: g.gui.%.md g.gui.%.tmp.md | $(MDDIR)
	VERSION_NUMBER=$(GRASS_VERSION_NUMBER) VERSION_DATE=$(GRASS_VERSION_DATE) MODULE_TOPDIR=$(MODULE_TOPDIR) \
        $(PYTHON) $(GISBASE)/utils/mkmarkdown.py g.gui.$* $(GRASS_VERSION_DATE) > $@

$(HTMLDIR)/wxGUI.%.html: g.gui.%.html | $(HTMLDIR)
	-rm -f g.gui.$*.tmp.html
	VERSION_NUMBER=$(GRASS_VERSION_NUMBER) VERSION_DATE=$(GRASS_VERSION_DATE) MODULE_TOPDIR=$(MODULE_TOPDIR) \
        $(PYTHON) $(GISBASE)/utils/mkhtml.py g.gui.$* $(GRASS_VERSION_DATE) > $@

$(MDDIR)/source/wxGUI.%.md: g.gui.%.md | $(MDDIR)
	-rm -f g.gui.$*.tmp.md
	VERSION_NUMBER=$(GRASS_VERSION_NUMBER) VERSION_DATE=$(GRASS_VERSION_DATE) MODULE_TOPDIR=$(MODULE_TOPDIR) \
        $(PYTHON) $(GISBASE)/utils/mkmarkdown.py g.gui.$* $(GRASS_VERSION_DATE) > $@

g.gui.%.tmp.html: $(SCRIPTDIR)/g.gui.%
	$(call htmldesc,$<,$@)

g.gui.%.tmp.md: $(SCRIPTDIR)/g.gui.%
	$(call mddesc,$<,$@)

$(SCRIPTDIR)/g.gui.%$(SCRIPTEXT): g.gui.%.py | $(SCRIPTDIR)
	$(INSTALL) $< $@

$(BIN)/g.gui.%.bat: $(MODULE_TOPDIR)/scripts/windows_launch.bat
	sed -e "s#SCRIPT_NAME#g.gui.$(*)#" -e "s#SCRIPT_DIR#%GISBASE%/scripts#" $(MODULE_TOPDIR)/scripts/windows_launch.bat > $@
	unix2dos $@

.PHONY: guiscript
