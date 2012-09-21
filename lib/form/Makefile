MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Vars.make

MOD_OBJS = generate.o open.o

LIB = FORM

include $(MODULE_TOPDIR)/include/Make/Lib.make

EXTRA_CFLAGS = $(XCFLAGS) $(TCLINCDIR) $(TKINCDIR)

FORMDIR=$(ARCH_DISTDIR)/etc/form
FORM=$(FORMDIR)/form.tcl
FORMPROG=$(FORMDIR)/form$(EXE)
HTMLLIB=$(FORMDIR)/html_library.tcl

ifneq ($(TCLTKLIBS),)
    GTCLTKFORM = $(HTMLLIB) $(FORM) $(FORMPROG) 
endif

# substitute OSX arch flags for tcltk
ifneq ($(MACOSX_ARCHS),)
CFLAGS := $(subst $(MACOSX_ARCHS),,$(CFLAGS)) $(MACOSX_ARCHS_TCLTK)
LDFLAGS := $(subst $(MACOSX_ARCHS),,$(LDFLAGS)) $(MACOSX_ARCHS_TCLTK)
endif

LIBES = $(DBMILIB) $(GISLIB) $(DATETIMELIB) $(TCLTKLIBPATH) $(TCLTKLIBS) \
	$(MATHLIB)

default: lib $(GTCLTKFORM)

$(FORMDIR):
	$(MKDIR) $@

$(HTMLLIB): html_library_grass.tcl | $(FORMDIR)
	$(INSTALL_DATA) $< $@

$(FORM): form.tcl | $(FORMDIR)
	$(INSTALL_DATA) $< $@

$(FORMPROG): $(OBJDIR)/form.o | $(FORMDIR)
	$(call linker_base,$(LINK),$(LDFLAGS),$(MANIFEST_OBJ) $(LIBES))
