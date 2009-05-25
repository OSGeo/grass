
PYTHON = python
PYMOD_LD = $(CXX) -shared
PYMOD_LDFLAGS = $(SHLIB_LDFLAGS) -L$(ARCH_LIBDIR)
ifeq ($(findstring darwin,$(ARCH)),darwin)
PYMOD_LDFLAGS := $(PYMOD_LDFLAGS) -bundle -undefined dynamic_lookup
else
PYMOD_LDFLAGS := $(PYMOD_LDFLAGS) $(PYTHONLDFLAGS)
endif
PYMOD_CFLAGS = $(SHLIB_CFLAGS) $(PYTHONINC) $(PYTHON_CFLAGS)

%.pyc: %.py
	$(PYTHON) -m py_compile $<

%_wrap.c %.py: %.i $(EXTRA_SWIG)
	$(SWIG) $(ARCH_INC) -python -module $* -shadow $<

_%.so: $(OBJDIR)/%_wrap.o $(_%_so_FILES)
	$(PYMOD_LD) -o $@ $(LDFLAGS) $(EXTRA_LDFLAGS) $(PYMOD_LDFLAGS) $(filter %.o,$^) $($*_LIBS) $(LIBES) $(EXTRA_LIBS)
