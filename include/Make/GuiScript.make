
# common dependencies and rules for building GUI module

include $(MODULE_TOPDIR)/include/Make/Vars.make
include $(MODULE_TOPDIR)/include/Make/Rules.make
include $(MODULE_TOPDIR)/include/Make/ScriptRules.make
include $(MODULE_TOPDIR)/include/Make/HtmlRules.make

MODULES  := $(patsubst g.gui.%.py,%,$(wildcard g.gui.*.py))
CMDHTML  := $(patsubst %,$(HTMLDIR)/g.gui.%.html,$(MODULES))
GUIHTML  := $(patsubst %,$(HTMLDIR)/wxGUI.%.html,$(MODULES))
PYFILES  := $(patsubst %,$(SCRIPTDIR)/g.gui.%,$(MODULES))

guiscript: $(CMDHTML) $(GUIHTML) $(IMGDST) $(PYFILES)

$(HTMLDIR)/g.gui.%.html: g.gui.%.html g.gui.%.tmp.html | $(HTMLDIR)
	$(PYTHON) $(GISBASE)/tools/mkhtml.py g.gui.$* $(GRASS_VERSION_DATE) > $@

$(HTMLDIR)/wxGUI.%.html: g.gui.%.html | $(HTMLDIR)
	-rm -f g.gui.$*.tmp.html
	$(PYTHON) $(GISBASE)/tools/mkhtml.py g.gui.$* $(GRASS_VERSION_DATE) > $@

g.gui.%.tmp.html: $(SCRIPTDIR)/g.gui.%
	$(call htmldesc,$<,$@)

$(SCRIPTDIR)/g.gui.%: g.gui.%.py | $(SCRIPTDIR)
	$(INSTALL) $< $@
