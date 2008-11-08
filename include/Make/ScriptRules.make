
# common dependencies and rules for building scripts

STRINGDIR = $(MODULE_TOPDIR)/locale/scriptstrings

$(SCRIPTDIR)/%: %.py
	if [ ! -d $(SCRIPTDIR) ]; then $(MKDIR) $(SCRIPTDIR); fi
	$(INSTALL) $< $@

$(SCRIPTDIR)/%: %
	if [ ! -d $(SCRIPTDIR) ]; then $(MKDIR) $(SCRIPTDIR); fi
	$(INSTALL) $< $@

$(BIN)/%.bat: $(MODULE_TOPDIR)/scripts/windows_launch.bat
	sed -e "s#SCRIPT_NAME#$*#" $(MODULE_TOPDIR)/scripts/windows_launch.bat > $@

# Make strings in a fake .c file so that they get picked up by the internationalizer stuff.
# These are only the options (parser.c) type things.
# See locale/scriptstrings/README for more information

strings = \
	GISRC=$(RUN_GISRC) \
	GISBASE=$(RUN_GISBASE) \
	PATH=$(BIN):$$PATH \
	$(LD_LIBRARY_PATH_VAR)="$(ARCH_LIBDIR):$($(LD_LIBRARY_PATH_VAR))" \
	g.parser -t $(1) | sed s/\"/\\\\\"/g | sed 's/.*/_("&")/' > \
	$(2) ; true

$(STRINGDIR)/%_to_translate.c: %.py
	$(call strings,$<,$@)

$(STRINGDIR)/%_to_translate.c: %
	$(call strings,$<,$@)
