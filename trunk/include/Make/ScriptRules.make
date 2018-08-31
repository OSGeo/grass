
# common dependencies and rules for building scripts

STRINGDIR = $(GRASS_HOME)/locale/scriptstrings

$(SCRIPTDIR)/%: %.py | $(SCRIPTDIR)
	$(INSTALL) $< $@

$(SCRIPTDIR)/%.py: %.py | $(SCRIPTDIR)
	$(INSTALL) $< $@

$(SCRIPTDIR):
	$(MKDIR) $(SCRIPTDIR)

$(ETCDIR)/%: % | $(ETCDIR)
	$(INSTALL_DATA) $< $@

$(ETCDIR):
	$(MKDIR) $(ETCDIR)

# Make strings in a fake .c file so that they get picked up by the internationalizer stuff.
# These are only the options (parser.c) type things.
# See locale/scriptstrings/README for more information

strings = $(call run_grass,g.parser -t $(1) | sed s/\"/\\\\\"/g | sed 's/.*/_("&")/' > $(2))

$(STRINGDIR)/%_to_translate.c: %.py
	-$(call strings,$<,$@)

$(STRINGDIR)/%_to_translate.c: %
	-$(call strings,$<,$@)
