MODULE_TOPDIR = ..

include $(MODULE_TOPDIR)/include/Make/Other.make
include $(MODULE_TOPDIR)/include/Make/Compile.make

GEM = gem$(GRASS_VERSION_MAJOR)$(GRASS_VERSION_MINOR)

default: $(ARCH_BINDIR)/$(GEM)$(EXE)

$(ARCH_BINDIR)/$(GEM)$(EXE): $(ARCH_OBJS)
	$(call linker_base,$(LINK),$(LDFLAGS) $(EXTRA_LDFLAGS),$(FMODE_OBJ) $(LIBES) $(EXTRA_LIBS))

install: $(UNIX_BIN)/$(GEM)$(EXE)

$(UNIX_BIN)/$(GEM)$(EXE): $(ARCH_BINDIR)/$(GEM)$(EXE)
	$(INSTALL)  $< $@
	-strip $@

.PHONY: install
