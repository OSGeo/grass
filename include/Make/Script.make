
SCRIPT = $(SCRIPTDIR)/$(PGM)

HTMLSRC = $(SCRIPT)

include $(MODULE_TOPDIR)/include/Make/Vars.make
include $(MODULE_TOPDIR)/include/Make/Rules.make
include $(MODULE_TOPDIR)/include/Make/Html.make
include $(MODULE_TOPDIR)/include/Make/ScriptRules.make

SCRIPT_ACTIONS = $(SCRIPT) html scriptstrings
ifdef MINGW
SCRIPT_ACTIONS += $(BIN)/$(PGM).bat
endif

script: $(SCRIPT_ACTIONS)

scriptstrings: $(STRINGDIR)/$(PGM)_to_translate.c

.PHONY: script scriptstrings
