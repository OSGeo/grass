
PACKAGE = "grassmods"

include $(MODULE_TOPDIR)/include/Make/Platform.make
include $(MODULE_TOPDIR)/include/Make/Grass.make
include $(MODULE_TOPDIR)/include/Make/Rules.make

PROGFILES = $(patsubst %,$(BIN)/%$(EXE),$(PROGRAMS))
HTMLFILES = $(patsubst %,$(HTMLDIR)/%.html,$(PROGRAMS))

multi: progs htmlmulti

progs: $(PROGFILES)

htmlmulti: $(HTMLFILES)

$(BIN)/%$(EXE): $(DEPENDENCIES)
	$(LINK) $(LDFLAGS) $(XTRA_LDFLAGS) -o $@ $(filter %.o,$^) $(FMODE_OBJ) $(LIBES) $(MATHLIB) $(XDRLIB)

define objs_rule
$(BIN)/$(1)$(EXE): $$(patsubst %.o,$(OBJDIR)/%.o,$$($$(subst .,_,$(1)_OBJS)))
$(HTMLDIR)/$(1).html: $(1).html $(1).tmp.html $(BIN)/$(1)$(EXE)
$(1).tmp.html: $(BIN)/$(1)$(EXE)
	$$(call htmldesc,$$<,$$@)
.INTERMEDIATE: $(1).tmp.html
endef

$(foreach prog,$(PROGRAMS),$(eval $(call objs_rule,$(prog))))
