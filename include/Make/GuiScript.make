
# common dependencies and rules for building GUI module

include $(MODULE_TOPDIR)/include/Make/Vars.make

include $(MODULE_TOPDIR)/include/Make/Rules.make
include $(MODULE_TOPDIR)/include/Make/ScriptRules.make
include $(MODULE_TOPDIR)/include/Make/HtmlRules.make

MODULES  := $(patsubst g.gui.%.py,%,$(wildcard g.gui.*.py))
CMDHTML  := $(patsubst %,$(HTMLDIR)/g.gui.%.html,$(MODULES))
GUIHTML  := $(patsubst %,$(HTMLDIR)/wxGUI.%.html,$(MODULES))
ifdef MINGW
SCRIPTEXT = .py
BATFILES  := $(patsubst %,$(BIN)/g.gui.%.bat,$(MODULES))
else
SCRIPTEXT =
BATFILES =
endif
PYFILES  := $(patsubst %,$(SCRIPTDIR)/g.gui.%$(SCRIPTEXT),$(MODULES))

guiscript: $(IMGDST) $(PYFILES) $(BATFILES)
	$(MAKE) $(CMDHTML)
	-rm -f g.gui.*.tmp.html
	$(MAKE) $(GUIHTML)

$(HTMLDIR)/g.gui.%.html: g.gui.%.html g.gui.%.tmp.html | $(HTMLDIR)
	VERSION_NUMBER=$(GRASS_VERSION_NUMBER) VERSION_DATE=$(GRASS_VERSION_DATE) MODULE_TOPDIR=$(MODULE_TOPDIR) \
        $(PYTHON) $(GISBASE)/tools/mkhtml.py g.gui.$* $(GRASS_VERSION_DATE) > $@

$(HTMLDIR)/wxGUI.%.html: g.gui.%.html | $(HTMLDIR)
	-rm -f g.gui.$*.tmp.html
	VERSION_NUMBER=$(GRASS_VERSION_NUMBER) VERSION_DATE=$(GRASS_VERSION_DATE) MODULE_TOPDIR=$(MODULE_TOPDIR) \
        $(PYTHON) $(GISBASE)/tools/mkhtml.py g.gui.$* $(GRASS_VERSION_DATE) > $@

g.gui.%.tmp.html: $(SCRIPTDIR)/g.gui.%
	$(call htmldesc,$<,$@)

$(SCRIPTDIR)/g.gui.%$(SCRIPTEXT): g.gui.%.py | $(SCRIPTDIR)
	$(INSTALL) $< $@

$(BIN)/g.gui.%.bat: $(MODULE_TOPDIR)/scripts/windows_launch.bat
	sed -e "s#SCRIPT_NAME#g.gui.$(*)#" -e "s#SCRIPT_DIR#%GISBASE%/scripts#" $(MODULE_TOPDIR)/scripts/windows_launch.bat > $@
	unix2dos $@

.PHONY: guiscript
