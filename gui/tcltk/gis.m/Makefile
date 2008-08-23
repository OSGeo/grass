MODULE_TOPDIR = ../../..

PGM=gis.m

include $(MODULE_TOPDIR)/include/Make/Script.make

default: script install_scripts

TCLFILES = $(patsubst %,$(ETC)/gm/%,$(wildcard *.tcl))
GIFFILES = $(patsubst %,$(ETC)/gm/%,$(wildcard *.gif))
DOCFILES = $(patsubst docs/%,$(HTMLDIR)/%,$(wildcard docs/gm_*.*))

$(HTMLDIR)/%: docs/%
	$(INSTALL_DATA) $< $@

$(ETC)/gm/%: %
	-test -d $(ETC)/gm || $(MKDIR) $(ETC)/gm
	$(INSTALL_DATA) $< $@

install_scripts: $(TCLFILES) $(GIFFILES) $(DOCFILES)

$(BIN)/$(PGM).bat: $(PGM).bat
	$(INSTALL_DATA) $< $@
