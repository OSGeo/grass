MODULE_TOPDIR = ..

SUBDIRS = timer g.html2man

include $(MODULE_TOPDIR)/include/Make/Dir.make
include $(MODULE_TOPDIR)/include/Make/Compile.make

default: parsubdirs $(TOOLSDIR)/mkhtml.py $(TOOLSDIR)/g.echo$(EXE)

$(TOOLSDIR)/mkhtml.py: mkhtml.py
	$(INSTALL) $< $@

$(TOOLSDIR)/g.echo$(EXE): $(OBJDIR)/g.echo.o
	$(call linker_base,$(LINK),$(LDFLAGS) $(EXTRA_LDFLAGS),$(MANIFEST_OBJ))
