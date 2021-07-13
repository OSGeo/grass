
# first found target
first: pre default

# create platform dirs 
ARCH_DIRS = $(ARCH_DISTDIR) $(ARCH_BINDIR) $(ARCH_INCDIR) $(ARCH_LIBDIR) \
	$(BIN) $(ETC) \
	$(DRIVERDIR) $(DBDRIVERDIR) $(FONTDIR) $(DOCSDIR) $(HTMLDIR) \
	$(MANBASEDIR) $(MANDIR) $(UTILSDIR)

pre: | $(ARCH_DIRS)

default:

$(ARCH_DIRS):
	$(MKDIR) $@

$(OBJDIR):
	-test -d $(OBJDIR) || $(MKDIR) $(OBJDIR)

$(ARCH_INCDIR)/%.h: %.h
	$(INSTALL_DATA) $< $@

ifneq ($(MINGW),)
ifdef CROSS_COMPILING
# build system is not MS Windows when cross-compiling
mkpath = $(1):$(2)
else
ifeq ($(wildcard $(UTILSDIR)/g.echo$(EXE)),)
# dummy path until g.echo.exe gets compiled and is needed
mkpath = $(1);$(2)
else
mkpath = $(shell $(UTILSDIR)/g.echo$(EXE) $(1));$(2)
endif
endif
else
mkpath = $(1):$(2)
endif

GRASS_PYTHONPATH := $(call mkpath,$(GISBASE)/gui/wxpython,$$PYTHONPATH)
GRASS_PYTHONPATH := $(call mkpath,$(GISBASE)/etc/python,$(GRASS_PYTHONPATH))
### really needed ???
### GRASS_PYTHONPATH := $(call mkpath,$(ARCH_DISTDIR)/etc/python,$(GRASS_PYTHONPATH))

run_grass = \
	GISRC=$(RUN_GISRC) \
	GISBASE=$(RUN_GISBASE) \
	PATH="$(ARCH_DISTDIR)/bin:$(GISBASE)/bin:$(GISBASE)/scripts:$$PATH" \
	PYTHONPATH="$(GRASS_PYTHONPATH)" \
	$(LD_LIBRARY_PATH_VAR)="$(BIN):$(GISBASE)/bin:$(GISBASE)/scripts:$(ARCH_LIBDIR):$(BASE_LIBDIR):$($(LD_LIBRARY_PATH_VAR))" \
	LC_ALL=C LANG=C LANGUAGE=C \
	$(1)

# default clean rules
clean:
	-rm -rf $(OBJDIR) $(EXTRA_CLEAN_DIRS)
	-rm -f $(EXTRA_CLEAN_FILES) *.tab.[ch] *.yy.c *.output *.backup *.tmp.html *.pyc $(DEPFILE)
	-if [ "$(CLEAN_SUBDIRS)" != "" ] ; then \
		list='$(CLEAN_SUBDIRS)' ; \
		for dir in $$list ; do \
			$(MAKE) -C $$dir clean ; \
		done ; \
	fi

depend:

.PHONY: clean depend
