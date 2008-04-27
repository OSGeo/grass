# common dependencies and rules for building shared libraries

SHLIB = $(ARCH_LIBDIR)/$(SHLIB_PREFIX)$(SHLIB_NAME).$(GRASS_VERSION_NUMBER)$(SHLIB_SUFFIX)

CFLAGS += $(SHLIB_CFLAGS) $(NLS_CFLAGS)
LDFLAGS += $(SHLIB_LDFLAGS)

$(SHLIB): $(SHLIB_OBJS)
	$(SHLIB_LD) -o $@ $(LDFLAGS) $^ $(EXTRA_LIBS) && \
	(cd $(ARCH_LIBDIR); ln -f -s $(notdir $@) $(patsubst %.$(GRASS_VERSION_NUMBER)$(SHLIB_SUFFIX),%$(SHLIB_SUFFIX),$@))

shlib: $(SHLIB)

