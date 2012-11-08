MODULE_TOPDIR = ../../..

PGM = g.gui.mapswipe
WXPGM = MapSwipe

include $(MODULE_TOPDIR)/include/Make/Script.make

default: script
	-rm -f $(PGM).tmp.html
	$(MAKE) $(HTMLDIR)/wxGUI.$(WXPGM).html

$(HTMLDIR)/wxGUI.$(WXPGM).html: $(PGM).html | $(HTMLDIR)
	$(PYTHON) $(GISBASE)/tools/mkhtml.py $(PGM) $(GRASS_VERSION_DATE) > $@
