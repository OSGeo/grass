MODULE_TOPDIR = ../..

PGM=grocat
include $(MODULE_TOPDIR)/include/Make/Etc.make
include $(MODULE_TOPDIR)/include/Make/NoHtml.make

TCLSRC := $(wildcard *.tcl)
TCLDST := $(patsubst %.tcl,$(ETC)/gtcltk/%.tcl,$(TCLSRC))

default: etc $(TCLDST)

$(ETC)/gtcltk/%.tcl: %.tcl | $(ETC)/gtcltk
	$(INSTALL_DATA) $< $@

$(ETC)/gtcltk:
	$(MKDIR) $@

