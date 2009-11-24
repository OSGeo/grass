MODULE_TOPDIR = ..

SUBDIRS = timer g.html2man

include $(MODULE_TOPDIR)/include/Make/Dir.make

default: parsubdirs $(TOOLSDIR)/mkhtml.py

$(TOOLSDIR)/mkhtml.py: mkhtml.py
	$(INSTALL) $< $@
