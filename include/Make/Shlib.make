# common dependencies and rules for building shared libraries

SHLIB = $(ARCH_LIBDIR)/$(SHLIB_PREFIX)$(SHLIB_NAME)$(SHLIB_SUFFIX)

CFLAGS += $(SHLIB_CFLAGS)
CXXFLAGS += $(SHLIB_CFLAGS)
LDFLAGS += $(SHLIB_LDFLAGS)

$(SHLIB): $(SHLIB_OBJS)
	$(SHLIB_LD) -o $@ $(LDFLAGS) $^ $(LIBES) $(EXTRA_LIBS) $(MATHLIB)
ifndef MINGW
	(cd $(ARCH_LIBDIR); ln -f -s $(notdir $@) $(patsubst %.$(GRASS_LIB_VERSION_NUMBER)$(SHLIB_SUFFIX),%$(SHLIB_SUFFIX),$@))
endif

shlib: $(SHLIB)
