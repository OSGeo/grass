
# common dependencies and rules for building scripts

SCRIPTDIR = $(ARCH_DISTDIR)/scripts
STRINGDIR = $(MODULE_TOPDIR)/locale/scriptstrings

SCRIPT = $(SCRIPTDIR)/$(PGM)

HTMLSRC = $(SCRIPT)

include $(MODULE_TOPDIR)/include/Make/Platform.make
include $(MODULE_TOPDIR)/include/Make/Grass.make
include $(MODULE_TOPDIR)/include/Make/Rules.make

SCRIPT_ACTIONS = $(SCRIPT) html scriptstrings
ifdef MINGW
SCRIPT_ACTIONS += $(BIN)/$(PGM).bat
endif

script: $(SCRIPT_ACTIONS)

$(SCRIPTDIR)/%: %.py
	if [ ! -d $(SCRIPTDIR) ]; then $(MKDIR) $(SCRIPTDIR); fi
	$(INSTALL) $< $@

$(SCRIPTDIR)/%: %
	if [ ! -d $(SCRIPTDIR) ]; then $(MKDIR) $(SCRIPTDIR); fi
	$(INSTALL) $< $@

$(BIN)/$(PGM).bat: $(MODULE_TOPDIR)/scripts/windows_launch.bat
	sed -e "s#SCRIPT_NAME#$(PGM)#" $(MODULE_TOPDIR)/scripts/windows_launch.bat > $@

# Make strings in a fake .c file so that they get picked up by the internationalizer stuff.
# These are only the options (parser.c) type things.
# See locale/scriptstrings/README for more information

$(STRINGDIR)/$(PGM)_to_translate.c: $(PGM)
	GISRC=$(RUN_GISRC) \
	GISBASE=$(RUN_GISBASE) \
	PATH=$(BIN):$$PATH \
	$(LD_LIBRARY_PATH_VAR)="$(ARCH_LIBDIR):$($(LD_LIBRARY_PATH_VAR))" \
	g.parser -t $(PGM) | sed s/\"/\\\\\"/g | sed 's/.*/_("&")/' > \
	$(STRINGDIR)/$(PGM)_to_translate.c ; true

scriptstrings: $(STRINGDIR)/$(PGM)_to_translate.c

.PHONY: script htmlscript scriptstrings
