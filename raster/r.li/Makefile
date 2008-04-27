MODULE_TOPDIR =../..

PGM = r.li

SUBDIRS1 = \
	r.li.edgedensity\
	r.li.patchdensity\
	r.li.patchnum\
	r.li.setup \
	r.li.shape\
	r.li.simpson\
	r.li.shannon\
	r.li.mps\
	r.li.mpa\
	r.li.padcv\
	r.li.padsd\
	r.li.padrange\
	r.li.cwed\
	r.li.richness\
	r.li.dominance\

SUBDIRS = r.li.daemon $(SUBDIRS1)

include $(MODULE_TOPDIR)/include/Make/Dir.make

default: parsubdirs
	$(INSTALL_DATA) description.html $(GISBASE)/docs/html/r.li.html

clean: cleansubdirs

$(SUBDIRS1): r.li.daemon

