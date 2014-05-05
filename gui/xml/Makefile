MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Dir.make

DSTDIR = $(GUIDIR)/xml

default: $(DSTDIR)/grass-interface.dtd 

$(DSTDIR)/grass-interface.dtd : grass-interface.dtd | $(DSTDIR)
	$(INSTALL_DATA) $< $@

$(DSTDIR):
	-test -d $@ | $(MKDIR) $@
