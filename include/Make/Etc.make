
# common dependencies and rules for building support programs

PACKAGE = "grassmods"

HTMLSRC = $(ETC)/$(PGM)$(EXE)

include $(MODULE_TOPDIR)/include/Make/Platform.make
include $(MODULE_TOPDIR)/include/Make/Grass.make
include $(MODULE_TOPDIR)/include/Make/Rules.make

etc: $(ETC)/$(PGM)$(EXE) html

$(ETC)/$(PGM)$(EXE): $(ARCH_CMD_OBJS) $(DEPENDENCIES) 
	$(CC) $(LDFLAGS) $(XTRA_LDFLAGS) -o $@ $(ARCH_CMD_OBJS) $(FMODE_OBJ) $(LIBES) $(MATHLIB) $(XDRLIB)

.PHONY: etc
