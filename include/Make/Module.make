
# common dependencies and rules for building module

PACKAGE = "grassmods"

HTMLSRC = $(BIN)/$(PGM)$(EXE)

include $(MODULE_TOPDIR)/include/Make/Vars.make
include $(MODULE_TOPDIR)/include/Make/Rules.make
include $(MODULE_TOPDIR)/include/Make/Html.make
include $(MODULE_TOPDIR)/include/Make/Compile.make

cmd: $(BIN)/$(PGM)$(EXE) html

$(BIN)/$(PGM)$(EXE): $(ARCH_OBJS) $(DEPENDENCIES)
	$(call linker)

install:
	$(INSTALL) $(ARCH_DISTDIR)/bin/$(PGM)$(EXE) $(INST_DIR)/bin/
	$(INSTALL_DATA) $(HTMLDIR)/$(PGM).html $(INST_DIR)/docs/html/
	$(INSTALL_DATA) $(ARCH_DISTDIR)/man/man1/$(PGM).1 $(INST_DIR)/man/man1/

.PHONY: cmd
