MODULE_TOPDIR =../..

PGM = r.li

SUBDIRS1 = \
	r.li.cwed \
	r.li.dominance \
	r.li.edgedensity \
	r.li.mpa \
	r.li.mps \
	r.li.padcv \
	r.li.padrange \
	r.li.padsd \
	r.li.patchdensity \
	r.li.patchnum \
	r.li.pielou \
	r.li.renyi \
	r.li.richness \
	r.li.shannon \
	r.li.shape \
	r.li.simpson
#	r.li.setup \

SUBDIRS = r.li.daemon $(SUBDIRS1)

include $(MODULE_TOPDIR)/include/Make/Dir.make

default: parsubdirs htmldir

$(SUBDIRS1): r.li.daemon

