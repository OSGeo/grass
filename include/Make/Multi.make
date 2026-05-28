
PACKAGE = "grassmods"

include $(MODULE_TOPDIR)/include/Make/Vars.make
include $(MODULE_TOPDIR)/include/Make/Rules.make
include $(MODULE_TOPDIR)/include/Make/Html.make
include $(MODULE_TOPDIR)/include/Make/Compile.make

PROGFILES = $(patsubst %,$(BIN)/%$(EXE),$(PROGRAMS))
HTMLFILES = $(patsubst %,$(HTMLDIR)/%.html,$(PROGRAMS))
MDFILES = $(patsubst %,$(MDDIR)/source/%.md,$(PROGRAMS))
MANFILES  = $(patsubst %,$(MANDIR)/%.$(MANSECT),$(PROGRAMS))

multi: progs htmlmulti

progs: $(PROGFILES)

ifdef CROSS_COMPILING
htmlmulti:
else
htmlmulti: $(HTMLFILES) $(MANFILES) $(MDFILES)
endif

$(BIN)/%$(EXE): $(DEPENDENCIES)
	$(call linker)

define objs_rule
$(BIN)/$(1)$(EXE): $$(patsubst %.o,$(OBJDIR)/%.o,$$($$(subst .,_,$(1)_OBJS)))
$(HTMLDIR)/$(1).html: $(1).html $(1).tmp.html $(BIN)/$(1)$(EXE)
$(1).tmp.html: $(BIN)/$(1)$(EXE)
	$$(call htmldesc,$$<,$$@)
$(MDDIR)/source/$(1).md: $(1).md $(1).tmp.md $(BIN)/$(1)$(EXE)
$(1).tmp.md: $(BIN)/$(1)$(EXE)
	$$(call mddesc,$$<,$$@)
.INTERMEDIATE: $(1).tmp.html $(1).tmp.md
endef

$(foreach prog,$(PROGRAMS),$(eval $(call objs_rule,$(prog))))
