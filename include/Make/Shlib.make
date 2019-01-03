# common dependencies and rules for building shared libraries

SHLIB = $(ARCH_LIBDIR)/$(SHLIB_PREFIX)$(SHLIB_NAME)$(SHLIB_SUFFIX)

CFLAGS += $(SHLIB_CFLAGS)
CXXFLAGS += $(SHLIB_CFLAGS)
LDFLAGS += $(SHLIB_LDFLAGS)

$(SHLIB): $(SHLIB_OBJS)
	$(SHLIB_LD) -o $@ $(LDFLAGS) $^ $(LIBES) $(EXTRA_LIBS) $(MATHLIB)
        # unversioned names are supposed to be symlinks, Windows
        # doesn't have symlinks, so MinGW's "ln" just copies the file
	(cd $(ARCH_LIBDIR); ln -f -s $(notdir $@) $(patsubst %.$(GRASS_VERSION_NUMBER)$(SHLIB_SUFFIX),%$(SHLIB_SUFFIX),$@))

shlib: $(SHLIB)
