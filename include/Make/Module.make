
# common dependencies and rules for building module

PACKAGE = "grassmods"

HTMLSRC = $(BIN)/$(PGM)$(EXE)

include $(MODULE_TOPDIR)/include/Make/Vars.make
include $(MODULE_TOPDIR)/include/Make/Rules.make
include $(MODULE_TOPDIR)/include/Make/Html.make
include $(MODULE_TOPDIR)/include/Make/Compile.make

ETCFILES_DST := $(patsubst %,$(ETC)/$(PGM)/%,$(ETCFILES))

cmd: $(BIN)/$(PGM)$(EXE) $(ETCFILES_DST) html

$(BIN)/$(PGM)$(EXE): $(ARCH_OBJS) $(DEPENDENCIES)
	$(call linker)

$(ETC)/$(PGM)/%: % | $(ETC)/$(PGM)
	$(INSTALL_DATA) $< $@

$(ETC)/$(PGM):
	$(MKDIR) $@

install:
	$(INSTALL) $(ARCH_DISTDIR)/bin/$(PGM)$(EXE) $(INST_DIR)/bin/
	$(INSTALL_DATA) $(HTMLDIR)/$(PGM).html $(INST_DIR)/docs/html/
	$(eval IMG := $(wildcard $(HTMLDIR)/*.png) $(wildcard $(HTMLDIR)/*.jpg) $(wildcard $(HTMLDIR)/*.gif))
	if [ -n "$(IMG)" ] ; then \
		$(INSTALL_DATA) $(IMG)  $(INST_DIR)/docs/html/ ; \
	fi
	$(INSTALL_DATA) $(ARCH_DISTDIR)/docs/man/man1/$(PGM).1 $(INST_DIR)/docs/man/man1/
	if [ -d "$(ETC)/$(PGM)" ] ; then \
		cp -rL $(ETC)/$(PGM) $(INST_DIR)/etc/ ; \
	fi

.PHONY: cmd
