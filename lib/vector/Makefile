MODULE_TOPDIR = ../..

SUBDIRS = rtree \
	dglib \
	diglib \
	Vlib \
	transform \
	vedit \
	neta

include $(MODULE_TOPDIR)/include/Make/Dir.make
include $(MODULE_TOPDIR)/include/Make/Doxygen.make

DOXNAME=vector

default: parsubdirs

diglib: rtree

Vlib: rtree dglib diglib

vedit: Vlib

neta: Vlib
