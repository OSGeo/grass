MODULE_TOPDIR = ../..

PGM = i.band.library

# TODO: unfortunately Script.make assumes that ETCFILES are Python
# modules only, this should be improved (by introducing a new variable
# in Script.make), there are more affected modules, see eg. d.polar or
# db.tests
JSON_FLS = $(wildcard *.json)
JSON_ETC = $(patsubst %,$(ETC)/$(PGM)/%,$(JSON_FLS))

include $(MODULE_TOPDIR)/include/Make/Script.make
include $(MODULE_TOPDIR)/include/Make/Python.make

default: script $(JSON_ETC)

$(ETC)/$(PGM)/%: % | $(ETC)/$(PGM)
	$(INSTALL_DATA) $< $@
