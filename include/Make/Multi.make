
PACKAGE = "grassmods"

include $(MODULE_TOPDIR)/include/Make/Platform.make
include $(MODULE_TOPDIR)/include/Make/Grass.make
include $(MODULE_TOPDIR)/include/Make/Rules.make

multi: progs inter htmlmulti

progs: $(patsubst %,$(BIN)/%$(EXE),$(PROGRAMS))

inter: $(patsubst %,%.tmp.html,$(PROGRAMS))

htmlmulti: $(patsubst %,$(HTMLDIR)/%.html,$(PROGRAMS))

%.tmp.html: $(BIN)/%$(EXE)
	$(call htmldesc,$<,$@)
