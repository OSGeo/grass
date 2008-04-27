MODULE_TOPDIR = ../..

SUBDIRS = rtree \
	dglib \
	diglib \
	Vlib \
	transform \
	vedit

#doxygen:
DOXNAME=vector

include $(MODULE_TOPDIR)/include/Make/Dir.make
include $(MODULE_TOPDIR)/include/Make/Doxygen.make

default: parsubdirs

clean: cleansubdirs

diglib: rtree

Vlib: rtree dglib diglib

vedit: Vlib
