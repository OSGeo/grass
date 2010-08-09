
ifeq ($(findstring darwin,$(ARCH)),darwin)
PYMOD_LD = $(CXX) -bundle -undefined dynamic_lookup
PYMOD_LDFLAGS := $(SHLIB_LDFLAGS) -L$(ARCH_LIBDIR)
else
PYMOD_LD = $(CXX) -shared
PYMOD_LDFLAGS := $(SHLIB_LDFLAGS) -L$(ARCH_LIBDIR) $(PYTHONLDFLAGS)
endif
PYMOD_CFLAGS = $(SHLIB_CFLAGS) $(PYTHONINC) $(PYTHON_CFLAGS)

PY_SOURCES := $(wildcard *.py)

%.pyc: %.py
	$(PYTHON) -m py_compile $<

%_wrap.c %.py: %.i $(EXTRA_SWIG)
	$(SWIG) $(INC) -python -module $* -shadow $<

_%.so: $(OBJDIR)/%_wrap.o $(_%_so_FILES)
	$(PYMOD_LD) -o $@ $(LDFLAGS) $(EXTRA_LDFLAGS) $(PYMOD_LDFLAGS) $(filter %.o,$^) $($*_LIBS) $(LIBES) $(EXTRA_LIBS)
