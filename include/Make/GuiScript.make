
# common dependencies and rules for building GUI module

include $(MODULE_TOPDIR)/include/Make/Vars.make

ifdef MINGW
SCRIPTEXT = .py
else
SCRIPTEXT = 
endif

include $(MODULE_TOPDIR)/include/Make/Rules.make
include $(MODULE_TOPDIR)/include/Make/ScriptRules.make
include $(MODULE_TOPDIR)/include/Make/HtmlRules.make

MODULES  := $(patsubst g.gui.%.py,%,$(wildcard g.gui.*.py))
CMDHTML  := $(patsubst %,$(HTMLDIR)/g.gui.%.html,$(MODULES))
GUIHTML  := $(patsubst %,$(HTMLDIR)/wxGUI.%.html,$(MODULES))
PYFILES  := $(patsubst %,$(SCRIPTDIR)/g.gui.%$(SCRIPTEXT),$(MODULES))

guiscript: $(IMGDST) $(PYFILES)
	$(MAKE) $(CMDHTML)
	-rm -f g.gui.*.tmp.html
	$(MAKE) $(GUIHTML)

$(HTMLDIR)/g.gui.%.html: g.gui.%.html g.gui.%.tmp.html | $(HTMLDIR)
	VERSION_NUMBER=$(GRASS_VERSION_NUMBER) VERSION_DATE=$(GRASS_VERSION_DATE) \
        $(PYTHON) $(GISBASE)/tools/mkhtml.py g.gui.$* $(GRASS_VERSION_DATE) > $@

$(HTMLDIR)/wxGUI.%.html: g.gui.%.html | $(HTMLDIR)
	-rm -f g.gui.$*.tmp.html
	VERSION_NUMBER=$(GRASS_VERSION_NUMBER) VERSION_DATE=$(GRASS_VERSION_DATE) \
        $(PYTHON) $(GISBASE)/tools/mkhtml.py g.gui.$* $(GRASS_VERSION_DATE) > $@

g.gui.%.tmp.html: $(SCRIPTDIR)/g.gui.%
	$(call htmldesc,$<,$@)

$(SCRIPTDIR)/g.gui.%$(SCRIPTEXT): g.gui.%.py | $(SCRIPTDIR)
	$(INSTALL) $< $@

.PHONY: guiscript
