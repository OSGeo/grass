
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
	$(eval IMG := $(wildcard $(HTMLDIR)/*.png) $(wildcard $(HTMLDIR)/*.jpg))
	if [ -n "$(IMG)" ] ; then \
		$(INSTALL_DATA) $(IMG)  $(INST_DIR)/docs/html/ ; \
	fi
	$(INSTALL_DATA) $(ARCH_DISTDIR)/docs/man/man1/$(PGM).1 $(INST_DIR)/docs/man/man1/
	if [ -d "$(ETC)/$(PGM)" ] ; then \
		cp -rL $(ETC)/$(PGM) $(INST_DIR)/etc/ ; \
	fi

.PHONY: cmd
