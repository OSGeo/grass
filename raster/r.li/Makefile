MODULE_TOPDIR =../..

PGM = r.li

SUBDIRS1 = \
	r.li.edgedensity \
	r.li.patchdensity \
	r.li.patchnum \
	r.li.shape \
	r.li.simpson \
	r.li.shannon \
	r.li.mps \
	r.li.mpa \
	r.li.padcv \
	r.li.padsd \
	r.li.padrange \
	r.li.cwed \
	r.li.richness \
	r.li.dominance
#	r.li.setup \

SUBDIRS = r.li.daemon $(SUBDIRS1)

include $(MODULE_TOPDIR)/include/Make/Dir.make

ifndef MINGW
default: parsubdirs htmldir
endif

$(SUBDIRS1): r.li.daemon

