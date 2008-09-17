MODULE_TOPDIR = ..

include $(MODULE_TOPDIR)/include/Make/Dir.make

# some definitions
SECT = 1
MANDIR  = $(ARCH_DISTDIR)/man/man$(SECT)
HTML2MAN = VERSION_NUMBER=${GRASS_VERSION_NUMBER} $(GRASS_HOME)/tools/g.html2man/g.html2man.py

MANPAGES := $(patsubst $(HTMLDIR)/%.html,$(MANDIR)/%.$(SECT),$(wildcard $(HTMLDIR)/*.html))

default: $(MANPAGES)

$(MANDIR):
	$(MKDIR) $(MANDIR)

ifneq ($(BROKEN_MAKE),)
$(MANDIR)/%.$(SECT): $(HTMLDIR)/%.html $(MANDIR)
else
$(MANDIR)/%.$(SECT): $(HTMLDIR)/%.html | $(MANDIR)
endif
	$(HTML2MAN) $< $@ $(SECT)

