
# common dependencies and rules for building module

PACKAGE ="grassmods"

include $(MODULE_TOPDIR)/include/Make/Platform.make
include $(MODULE_TOPDIR)/include/Make/Grass.make
include $(MODULE_TOPDIR)/include/Make/Rules.make

cmd: $(BIN)/$(PGM)$(EXE)
	$(MAKE) htmlcmd

$(BIN)/$(PGM)$(EXE): $(ARCH_CMD_OBJS) $(DEPENDENCIES) 
	$(CC) $(LDFLAGS) $(XTRA_LDFLAGS) -o $@ $(ARCH_CMD_OBJS) $(FMODE_OBJ) $(LIBES) $(MATHLIB) $(XDRLIB)

etc: $(ETC)/$(PGM)$(EXE)
	$(MAKE)  htmletc

$(ETC)/$(PGM)$(EXE): $(ARCH_CMD_OBJS) $(DEPENDENCIES) 
	$(CC) $(LDFLAGS) $(XTRA_LDFLAGS) -o $@ $(ARCH_CMD_OBJS) $(FMODE_OBJ) $(LIBES) $(MATHLIB) $(XDRLIB)

.PHONY: cmd etc
