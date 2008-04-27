
MODULE_TOPDIR = ../..

SUBDIRS1 = \
	r.sim.water \
	r.sim.sediment

SUBDIRS = simlib $(SUBDIRS1)

include $(MODULE_TOPDIR)/include/Make/Dir.make

default: parsubdirs

clean: cleansubdirs

$(SUBDIRS1): simlib

