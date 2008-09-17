MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Vars.make

MOD_OBJS = generate.o open.o

LIB_NAME = $(FORM_LIBNAME)

include $(MODULE_TOPDIR)/include/Make/Lib.make

EXTRA_LIBS=$(GISLIB) $(DBMILIB)
EXTRA_CFLAGS = $(XCFLAGS) $(TCLINCDIR) $(TKINCDIR)

FORMDIR=$(ARCH_DISTDIR)/etc/form
FORM=$(FORMDIR)/form.tcl
FORMPROG=$(FORMDIR)/form
HTMLLIB=$(FORMDIR)/html_library.tcl

ifneq ($(TCLTKLIBS),)
    GTCLTKFORM = $(HTMLLIB) $(FORM) $(FORMPROG) 
endif

default: lib $(GTCLTKFORM)

$(FORMDIR):
	if [ ! -d $@ ]; then $(MKDIR) $@; fi

$(HTMLLIB): html_library_grass.tcl
	$(MAKE) $(FORMDIR)
	$(INSTALL_DATA) $< $@

$(FORM): form.tcl
	$(MAKE) $(FORMDIR)
	$(INSTALL_DATA) $< $@

$(FORMPROG): $(OBJDIR)/form.o
	$(MAKE) $(FORMDIR)
	$(CC) $(LDFLAGS) -o $@ $(OBJDIR)/form.o $(DBMILIB) $(GISLIB) $(DATETIMELIB) \
				$(TCLTKLIBPATH) $(TCLTKLIBS) $(MATHLIB) $(XDRLIB)
