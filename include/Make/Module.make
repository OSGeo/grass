
# common dependencies and rules for building module

PACKAGE = "grassmods"

HTMLSRC = $(BIN)/$(PGM)$(EXE)

include $(MODULE_TOPDIR)/include/Make/Platform.make
include $(MODULE_TOPDIR)/include/Make/Grass.make
include $(MODULE_TOPDIR)/include/Make/Rules.make

cmd: $(BIN)/$(PGM)$(EXE) html

$(BIN)/$(PGM)$(EXE): $(ARCH_CMD_OBJS) $(DEPENDENCIES)
	$(LINK) $(LDFLAGS) $(XTRA_LDFLAGS) -o $@ $(ARCH_CMD_OBJS) $(FMODE_OBJ) $(LIBES) $(MATHLIB) $(XDRLIB)

.PHONY: cmd
